
#include "anet.h"
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ae.h"
#include <assert.h>
#include <stdlib.h>

int anetSetBlock(char *err, int fd, int non_block)
{
    int flags;

    if ((flags = fcntl(fd, F_GETFL)) == -1)
    {
        // anetSetError(err, "fcntl(F_GETFL): %s", strerror(errno));
        return ANET_ERR;
    }

    if (non_block)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flags) == -1)
    {
        // anetSetError(err, "fcntl(F_SETFL,O_NONBLOCK): %s", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
}

int anetNonBlock(char *err, int fd)
{
    return anetSetBlock(err, fd, 1);
}

int anetTcpServer(char *err, int port, char *bindaddr, int backlog)
{
    struct  sockaddr_in address;
    address.sin_family = AF_INET;
    inet_pton(AF_INET, bindaddr, &address.sin_addr);
    address.sin_port = htons(port);
    
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    int ret = bind(sockfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sockfd, backlog);
    assert(ret != -1);
    return sockfd;
}