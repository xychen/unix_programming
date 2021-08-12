#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 1023

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

int unblock_connect(const char *ip, int port, int time)
{
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    int fdopt = setnonblocking(sockfd);
    //连接
    ret = connect(sockfd, (struct sockaddr *)&address, sizeof(address));
    if (ret == 0)
    {
        //如果连接成功，则回复sockfd的属性，并立即返回
        printf("connect with server immediately\n");
        fcntl(sockfd, F_SETFL, fdopt;
        return sockfd;
    }
    else if (errno != EINPROGRESS)
    {
        //如果连接没有立即简历，那么只有当errno是EINPROGRESS的时候才表示连接还在进行，否则出错返回
        printf("unblock connect not support\n");
        return -1;
    }

    fd_set readfds;
    fd_set writefds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FD_SET(sockfd, &writefds);

    timeout.tv_sec = time;
    timeout.tv_usec = 0;

    ret = select(sockfd + 1, NULL, &writefds, NULL, &timeout);
    if (ret <= 0)
    {
        printf("connection time out\n");
        close(sockfd);
        return -1;
    }

    if (!FD_ISSET(sockfd, &writefds))
    {
        printf("no events on sockfd found\n");
        close(sockfd);
        return -1;
    }

    int error = 0;
    socklen_t length = sizeof(error);
    //调用getsockopt来获取并清除sockfd上的错误
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &length) < 0)
    {
        printf("get socket option failed\n");
        close(sockfd);
        return -1;
    }
    //错误号不为 0 表示连接出错
    if (error != 0)
    {
        printf("connection failed after select with the error: %d\n", error);
        close(sockfd);
        return -1;
    }
    //连接成功
    printf("connection ready after select with the socket: %d\n", sockfd);
    fcntl(sockfd, F_SETFL, fdopt);
    return sockfd;
}

int main(int argc, char *argv[])
{
    if (argc <= 2)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int sockfd = unblock_connect(ip, port, 10);
    if (sockfd < 0)
    {
        return 1;
    }
    close(sockfd);
    return 0;
}