
#include "ae.h"
#include "ae_epoll.c"
#include <unistd.h>

aeEventLoop *aeCreateEventLoop(int setsize)
{
    aeEventLoop *el = (aeEventLoop *)malloc(sizeof(aeEventLoop));
    // el->maxfd = setsize;
    el->setsize = setsize;
    //利用fd做index， 内容是aeFileEvent
    el->events = (aeFileEvent *)malloc(setsize * sizeof(aeFileEvent));
    //初始化
    for (int i = 0; i < setsize; i++)
        el->events[i].mask = AE_NONE;

    el->fired = (aeFiredEvent *)malloc(setsize * sizeof(aeFileEvent));
    //创建epoll结构体
    aeApiCreate(el);
    return el;
}

int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask, aeFileProc *proc, void *clientData)
{
    //fd超出了范围
    if (fd > eventLoop->setsize)
    {
        errno = ERANGE;
        return AE_ERR;
    }
    //添加到epoll事件中
    if (aeApiAddEvent(eventLoop, fd, mask) == -1)
    {
        return AE_ERR;
    }

    aeFileEvent *fe = &eventLoop->events[fd];
    fe->mask |= mask;
    if (mask & AE_READABLE)
    {
        fe->rfileProc = proc;
    }

    if (mask & AE_WRITABLE)
    {
        fe->wfileProc = proc;
    }

    fe->clientData = clientData;

    // 设定maxfd
    // if (fd > eventLoop->maxfd)
    // {
    //     eventLoop->maxfd = fd;
    // }

    return 0;
}

void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask)
{
    if (fd >= eventLoop->setsize)
        return;
    aeFileEvent *fe = &eventLoop->events[fd];
    if (fe->mask == AE_NONE)
        return;
    aeApiDelEvent(eventLoop, fd, mask);
    fe->mask |= ~mask;

    //@todo: 重新设定maxfd
}

//事件处理
int aeProcessEvents(aeEventLoop *eventLoop, int flags)
{
    int numevents = 0;
    numevents = aeApiPoll(eventLoop, NULL);
    for (int i = 0; i < numevents; i++)
    {
        int fd = eventLoop->fired[i].fd;
        int mask = eventLoop->fired[i].mask;
        aeFileEvent *fe = &eventLoop->events[fd];

        //连接注册了读事件，且触发了读事件  （fe->mask & AE_READABLE）&& (mask & AE_READABLE)
        if (fe->mask & mask & AE_READABLE)
        {
            fe->rfileProc(eventLoop, fd, fe->clientData, fe->mask);
        }

        if (fe->mask & mask & AE_WRITABLE)
        {
            fe->wfileProc(eventLoop, fd, fe->clientData, fe->mask);
        }

        //@todo: 处理 AE_BARRIER 事件
    }
}

void aeMain(aeEventLoop *eventLoop)
{
    while (1)
    {
        aeProcessEvents(eventLoop, 0);
    };
}