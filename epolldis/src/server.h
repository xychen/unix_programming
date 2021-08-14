
#ifndef __EPOLLDIS_H
#define __EPOLLDIS_H

#include "ae.h"
#include <unistd.h>

#define PROTO_REPLY_CHUNK_BYTES (16 * 1024) /* 16k output buffer */
#define MAX_CLIENTS 65535

typedef struct client
{
    int fd;                 //文件描述符
    char *querybuf;         // Buffer we use to accumulate client queries.
    size_t qb_pos;          // The position we have read in querybuf.
    char *pending_querybuf; // If this client is flagged as master, this buffer

    /* Response buffer */
    int bufpos;
    char buf[PROTO_REPLY_CHUNK_BYTES];
} client;

struct epolldisServer
{
    aeEventLoop *el;
    int num_connections; //已连接的客户端数量
    client **client_list;
    int max_clients;
    char *ip;
    int port;
};

extern struct epolldisServer server;

void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask);
void linkClient(client *c);
void unlinkClient(client *c);
client *createClient(int fd);
void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask);
void sendReplyToClient(aeEventLoop *el, int fd, void *privdata, int mask);
void processInputBuffer(client *c);

#endif