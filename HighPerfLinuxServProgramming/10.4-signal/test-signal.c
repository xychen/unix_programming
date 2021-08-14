
#include <stdio.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <strings.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>

#define MAX_EVENT_NUMBER 1024
static int pipefd[2];

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

//信号处理函数
void sig_handler(int sig)
{
    //保留原来的errno，在函数最后回复
    int save_errno = errno;
    int msg = sig;
    //将信号值写入管道，以通知主循环
    send(pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

void addsig(int sig)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
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

    struct epoll_event events[MAX_EVENT_NUMBER];

    int epollfd = epoll_create(5);
    assert(epollfd != -1);

    addfd(epollfd, sockfd);

    //使用socketpair创建管道，注册pipefd[0]上的可读事件
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);
    setnoblock(pipefd[1]);
    addfd(epollfd, pipefd[0]);

    //设置一些信号的处理函数
    addsig(SIGHUP);
    addsig(SIGCHLD);
    addsig(SIGTERM);
    addsig(SIGINT);
    bool stop_server = false;

    while (!stop_server)
    {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR))
        {
            printf("epoll failure - %d\n", number);
            break;
        }
        int i;
        for (i = 0; i < number; i++)
        {
            int fd = events[i].data.fd;

            if (fd == sockfd)
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
            else if ((fd == pipefd[0]) && (events[i].events & EPOLLIN))
            {
                //如果就绪的文件描述符是pipefd[0]，则处理信号
                int sig;
                char signals[1024];
                ret = recv(pipefd[0], signals, sizeof(signal), 0);
                if (ret == -1)
                {
                    continue;
                }
                else if (ret == 0)
                {
                    continue;
                }
                else
                {
                    //因为每个信号值占1字节，所以按照字节来逐个接收信号
                    int i;
                    for (i = 0; i < ret; ++i)
                    {
                        switch (signals[i])
                        {
                        case SIGCHLD:
                        case SIGHUP:
                        {
                            continue;
                        }
                        case SIGTERM:
                        case SIGINT:
                        {
                            printf("get signals - %d\n", signals[i]);
                            stop_server = true; //修改循环变量的值
                        }
                        }
                    }
                }
            }
            else
            {
                printf("something else\n");
            }
        }
    }

    printf("close fds\n");
    close(sockfd);
    close(pipefd[1]);
    close(pipefd[0]);

    return 0;
}