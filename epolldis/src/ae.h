

#ifndef __AE_H
#define __AE_H

#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define AE_OK 0
#define AE_ERR -1

#define AE_NONE 0     /* No events registered. */
#define AE_READABLE 1 /* Fire when descriptor is readable. */
#define AE_WRITABLE 2 /* Fire when descriptor is writable. */
#define AE_BARRIER 4  /* With WRITABLE, never fire the event if the */

struct aeEventLoop;

typedef void aeFileProc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);

typedef struct aeFileEvent
{
    int mask;
    aeFileProc *rfileProc;
    aeFileProc *wfileProc;
    void *clientData;
} aeFileEvent;

typedef struct aeFiredEvent
{
    int mask;
    int fd;
} aeFiredEvent;

typedef struct aeEventLoop
{
    int maxfd;
    int setsize;
    aeFileEvent *events;
    aeFiredEvent *fired;
    void *apidata;
} aeEventLoop;

aeEventLoop *aeCreateEventLoop(int setsize);
int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask, aeFileProc *proc, void *clientData);
void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask);
int aeProcessEvents(aeEventLoop *eventLoop, int flags);
void aeMain(aeEventLoop *eventLoop);

#endif