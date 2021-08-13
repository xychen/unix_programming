#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>

#define MAX_FD_NUMBER 65535
#define USER_CONN_LIMIT 5
#define BUFFER_SIZE 100

typedef struct
{
    int client_fd;
    struct epoll_event event;
    char *readbuf;
    char writebuf[BUFFER_SIZE];
} client;

int setnonblock(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void unlinkClient(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
}

void mainloop(int epollfd, int sockfd)
{
    setnonblock(sockfd);
    struct epoll_event event;
    event.data.fd = sockfd;
    event.events = EPOLLIN | EPOLLET;

    //添加到epoll中
    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event);
    client *client_list = (client *)malloc(MAX_FD_NUMBER * sizeof(client));
    struct epoll_event fired_events[MAX_FD_NUMBER];
    int user_connect_nums = 0;
    int userfds[USER_CONN_LIMIT];

    while (1)
    {
        // printf("start epoll_wait:\n");
        int fired_num = epoll_wait(epollfd, fired_events, MAX_FD_NUMBER, -1);
        // printf("event fired\n");
        int i = 0;
        for (i = 0; i < fired_num; i++)
        {
            int fd = fired_events[i].data.fd;
            // printf("epoll event value: %x, fd: %d\n", fired_events[i].events, fd);
            if (client_list[fd].client_fd == -1)
            {
                continue;
            }
            if (fd == sockfd) //有新的连接过来
            {
                struct sockaddr_in client_address;
                socklen_t len = sizeof(client_address);
                int connfd = accept(sockfd, (struct sockaddr *)&client_address, &len);
                if (connfd <= 0)
                {
                    printf("bad client connection");
                    continue;
                }
                printf("get new connection - %d\n", connfd);
                //连接数太多
                if (user_connect_nums >= USER_CONN_LIMIT)
                {
                    const char *msg = "too many connections\n";
                    send(connfd, msg, strlen(msg), 0);
                    close(connfd);
                    continue;
                }
                setnonblock(connfd);
                //存放连接
                client_list[connfd].client_fd = connfd;
                client_list[connfd].event.data.fd = connfd;
                client_list[connfd].event.events = EPOLLIN | EPOLLET;
                userfds[user_connect_nums] = connfd;
                //放到epoll中
                epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &client_list[connfd].event);
                user_connect_nums++;
                continue;
            }

            //读请求
            if (fired_events[i].events & EPOLLIN)
            {
                client_list[fd].readbuf = (char *)malloc((BUFFER_SIZE + 1) * sizeof(char));
                memset(client_list[fd].readbuf, '\0', BUFFER_SIZE + 1);
                int readlen = recv(fd, client_list[fd].readbuf, BUFFER_SIZE, 0);
                // printf("read len: %d\n", readlen);
                if (readlen == -1)
                {
                    if (errno == EAGAIN)
                    {
                        continue;
                    }
                    else
                    {
                        unlinkClient(epollfd, fd);
                        userfds[i] = userfds[user_connect_nums];
                        client_list[fd].client_fd = -1;
                        user_connect_nums--;
                        i--;
                        continue;
                    }
                }
                else if (readlen == 0) //对方关闭时，读到的数据会是0
                {
                    unlinkClient(epollfd, fd);
                    client_list[fd].client_fd = -1;
                    userfds[i] = userfds[user_connect_nums];
                    user_connect_nums--;
                    i--;
                    continue;
                }
                else if (readlen > 0)
                {
                    printf("get message: %s", client_list[fd].readbuf);
                    //将消息发给每一个其他的连接
                    int j;
                    for (j = 0; j < user_connect_nums; j++)
                    {
                        int client_fd = userfds[j];
                        // printf("foreach all clients: %d - %d\n", fd, client_fd);
                        if (client_fd == fd) //不给自己回显
                        {
                            continue;
                        }
                        //处理消息，此处是直接复制消息（可以改成其他操作）
                        strcpy(client_list[client_fd].writebuf, client_list[fd].readbuf);
                        // printf("to send: %s - %d\n", client_list[client_fd].writebuf, client_fd);
                        //暂时移除读事件，添加写事件
                        client_list[client_fd].event.events |= ~EPOLLIN;
                        client_list[client_fd].event.events |= EPOLLOUT;

                        //为啥没生效
                        int ret = epoll_ctl(epollfd, EPOLL_CTL_MOD, client_fd, &client_list[client_fd].event);
                        // printf("epoll ctl: %d\n");
                    }
                    client_list[fd].readbuf = NULL;
                }
                continue;
            }

            //写请求
            if (fired_events[i].events & EPOLLOUT)
            {
                int sendlen = send(fd, client_list[fd].writebuf, strlen(client_list[fd].writebuf), 0);
                printf("send msg to fd(%d) - %s\n", sendlen, client_list[fd].writebuf);
                if (sendlen == -1)
                {
                    //@todo:写入失败
                }
                client_list[fd].writebuf[0] = '\0';
                client_list[fd].event.events |= ~EPOLLOUT;
                client_list[fd].event.events |= EPOLLIN;
                epoll_ctl(epollfd, EPOLL_CTL_MOD, client_list[fd].client_fd, &client_list[fd].event);

                continue;
            }
        }
    }

    delete (client_list);
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

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    int ret = bind(sockfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sockfd, 5);
    assert(ret != -1);

    int epollfd = epoll_create(1);
    assert(epollfd != -1);

    //主循环
    mainloop(epollfd, sockfd);

    close(sockfd);
    return 0;
}