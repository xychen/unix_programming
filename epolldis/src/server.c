#include <stdio.h>
#include "server.h"
#include <libgen.h>
#include "ae.h"
#include <sys/socket.h>
#include "anet.h"
#include <stdlib.h>

//global
struct epolldisServer server;

void initServerConfig(char *ip, int port)
{
    server.num_connections = 0;
    server.el = aeCreateEventLoop(MAX_CLIENTS);
    server.client_list = (client **)malloc(MAX_CLIENTS * sizeof(client *));
    
    server.max_clients = MAX_CLIENTS;
    server.ip = ip;
    server.port = port;
}

int listenToPort()
{
    //@todo: error
    //@todo: backlog设置的1000
    int sockfd = anetTcpServer(NULL, server.port, server.ip, 1000);
    anetNonBlock(NULL, sockfd);

    //绑定事件
    if(aeCreateFileEvent(server.el, sockfd, AE_READABLE, acceptTcpHandler, NULL) == AE_ERR)
    {
        //@todo: error
    }
}

void initServer(void)
{
    listenToPort();
}

int main(int argc, char *argv[])
{
    if (argc <= 2)
    {
        printf("usage: %s ip_address port_number", basename(argv[0]));
        return 1;
    }
    char *ip = argv[1];
    int port = atoi(argv[2]);
    initServerConfig(ip, port);
    initServer();
    aeMain(server.el);
    return 0;
}