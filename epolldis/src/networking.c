
#include "ae.h"
#include "server.h"
#include "anet.h"
#include <stdlib.h>

void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask)
{
}

//添加连接
void linkClient(client *c)
{
    server.client_list[server.num_connections] = c;
    server.num_connections++;
}

//删除连接
void unlinkClient(client *c)
{
    if (c->fd != -1)
    {
        //从连接链表中删除
        for (int i = 0; i < server.num_connections; i++)
        {
            if (server.client_list[i]->fd == c->fd)
            {
                server.client_list[i] = server.client_list[server.num_connections];
                server.num_connections--;
                aeDeleteFileEvent(server.el, c->fd, AE_READABLE);
                aeDeleteFileEvent(server.el, c->fd, AE_WRITABLE);
                break;
            }
        }
        close(c->fd);
        c->fd = -1;
    }
}

//创建新的客户端
client *createClient(int fd)
{
    client *new_client = (client *)malloc(sizeof(client));
    new_client->fd = fd;
    anetNonBlock(NULL, fd);
    //todo： set no delay
    //todo: keepalive

    if (aeCreateFileEvent(server.el, fd, AE_READABLE, readQueryFromClient, new_client) == AE_ERR)
    {
        close(fd);
        free(new_client);
        return NULL;
    }
    //todo: buf相关的
    linkClient(new_client);
    //放到server的数组里边
    return new_client;
}

//接受连接
void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask)
{
    struct sockaddr_in client_address;
    socklen_t len = sizeof(client_address);
    int new_connfd = accept(fd, (struct sockaddr *)&client_address, &len);
    if (new_connfd <= 0)
    {
        return;
    }

    //@todo: 判断最大连接数

    aeFileEvent *fe = &el->events[new_connfd];
    //创建新的client
    fe->clientData = createClient(new_connfd);
}

/* Write event handler. Just send data to the client. */
void sendReplyToClient(aeEventLoop *el, int fd, void *privdata, int mask)
{
    // UNUSED(el);
    // UNUSED(mask);
    // writeToClient(fd,privdata,1);
}

//解析命令
void processInputBuffer(client *c)
{
}