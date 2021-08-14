
#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define MAX_EVENT_NUMBER 1024
#define TCP_BUFFER_SIZE 512
#define UPD_BUFFER_SIZE 1024

int setnoblock(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnoblock(fd);
}

void delfd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
}

int main(int argc, char *argv[])
{
    if (argc <= 2)
    {
        printf("usage: %s ip_address port\n", basename(argv[0]));
        return 1;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    //创建tcp socket
    struct sockaddr_in address;
    bzero(&address, sizeof(address));

    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    int ret = bind(sockfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sockfd, 5);
    assert(ret != -1);

    //创建udp socket
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port + 1);

    int udpfd = socket(PF_INET, SOCK_DGRAM, 0);
    assert(udpfd >= 0);

    ret = bind(udpfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    struct epoll_event events[MAX_EVENT_NUMBER];

    int epollfd = epoll_create(5);
    assert(epollfd != -1);

    addfd(epollfd, sockfd);
    addfd(epollfd, udpfd);

    while (1)
    {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0)
        {
            printf("epoll failure\n");
            break;
        }
        for (int i = 0; i < number; i++)
        {
            int fd = events[i].data.fd;
            if (fd == sockfd) //tcp连接请求
            {
                struct sockaddr_in client_address;
                socklen_t len = sizeof(client_address);
                int connfd = accept(fd, (struct sockaddr *)&client_address, &len);
                if (connfd == -1)
                {
                    printf("bad connection\n");
                    continue;
                }
                printf("new tcp connection: %d\n", connfd);
                addfd(epollfd, connfd);
            }
            else if (fd == udpfd) //udp数据
            {
                char buf[UPD_BUFFER_SIZE];
                memset(buf, '\0', UPD_BUFFER_SIZE);
                struct sockaddr_in client_address;
                socklen_t len = sizeof(client_address);
                ret = recvfrom(udpfd, buf, UPD_BUFFER_SIZE - 1, 0, (struct sockaddr *)&address, &len);
                if (ret > 0)
                {
                    sendto(udpfd, buf, UPD_BUFFER_SIZE - 1, 0, (struct sockaddr *)&address, len);
                }
            }
            else if(events[i].events & EPOLLIN)     //tcp数据
            {
                char buf[TCP_BUFFER_SIZE];
                while(1)
                {
                    memset(buf, '\0', TCP_BUFFER_SIZE);
                    ret = recv(fd, buf, TCP_BUFFER_SIZE - 1, 0);
                    if(ret < 0)
                    {
                        if((errno == EAGAIN) || (errno == EWOULDBLOCK))
                        {
                            break;
                        }
                        delfd(epollfd, fd);
                        close(fd);
                        break;
                    }
                    else if(ret == 0)       //对端已经关闭了
                    {
                        delfd(epollfd, fd);
                        close(fd);
                    }
                    else
                    {
                        send(fd, buf, ret, 0);
                    }
                }
            }
            else
            {
                printf("something else happened\n");
            }

        }
    }
    close(sockfd);
    // close(udpfd);
    return 0;
}