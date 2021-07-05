/**
 * epoll实现的服务端
 * 参考文章：http://zhoulifa.bokee.com/6081520.html
 * 编译程序用下列命令：gcc -Wall epoll-server.c -o epoll-server
 * 运行程序用如下命令：
 * 服务端运行： ./epoll-server 10000 2 127.0.0.1
 * 客户端运行：nc 127.0.0.1 10000
 * */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
// #include <openssl/ssl.h>
// #include <openssl/err.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/resource.h>

#define MAXBUF 1024
#define MAXEPOLLSIZE 10000

int setnonblocking(int sockfd)
{
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK) == -1)
    {
        return -1;
    }
    return 0;
}

int handle_message(int new_fd)
{
    char buf[MAXBUF + 1];
    int len;
    bzero(buf, MAXBUF + 1);
    len = recv(new_fd, buf, MAXBUF, 0);
    if (len > 0)
    {
        printf("%d 接受消息成功：%s, 共%d个字节的数据\n", new_fd, buf, len);
    }
    else
    {
        if (len < 0)
        {
            printf("消息接收失败！错误代码是%d，错误信息是'%s'\n", errno, strerror(errno));
        }
        close(new_fd);
        return -1;
    }
    return len;
}

int main(int argc, char **argv)
{
    int listen_fd, new_fd, epoll_fd, epoll_fired_events_num, n, ret, curfds;
    socklen_t len;
    struct sockaddr_in my_addr, their_addr;
    unsigned int myport, lisnum;
    struct epoll_event ev;
    struct epoll_event fired_events[MAXEPOLLSIZE];
    struct rlimit rt;

    if (argv[1])
    {
        myport = atoi(argv[1]);
    }
    else
    {
        myport = 7838;
    }

    if (argv[2])
    {
        lisnum = atoi(argv[2]);
    }
    else
    {
        lisnum = 2;
    }

    //设置每个进程允许打开的最大文件数
    rt.rlim_max = rt.rlim_cur = MAXEPOLLSIZE;
    if (setrlimit(RLIMIT_NOFILE, &rt) == -1)
    {
        perror("setrlimit");
        exit(1);
    }
    else
    {
        printf("设置系统资源参数成功！\n");
    }

    //开启socket监听
    if ((listen_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }
    else
    {
        printf("socket 创建成功！\n");
    }

    setnonblocking(listen_fd);

    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = PF_INET;
    my_addr.sin_port = htons(myport);
    if (argv[3])
    {
        my_addr.sin_addr.s_addr = inet_addr(argv[3]);
    }
    else
    {
        my_addr.sin_addr.s_addr = INADDR_ANY;
    }

    if (bind(listen_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("bind");
        exit(1);
    }
    else
    {
        printf("IP 地址和端口绑定成功\n");
    }

    if (listen(listen_fd, lisnum) == -1)
    {
        perror("listen");
        exit(1);
    }
    else
    {
        printf("开启服务成功！\n");
    }

    //创建epoll句柄，把监听socket假如到epoll集合里
    epoll_fd = epoll_create(MAXEPOLLSIZE);
    len = sizeof(struct sockaddr_in);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) < 0)
    {
        fprintf(stderr, "epoll set insertion error: fd=%d\n", listen_fd);
        return -1;
    }
    else
    {
        printf("监听 socket 加入 epoll 成功！\n");
    }

    curfds = 1;
    while (1)
    {
        //等待有事件发生，fired_events中存储已经触发的事件，-1表示没有超时时间，返回触发的事件数量
        epoll_fired_events_num = epoll_wait(epoll_fd, fired_events, curfds, -1);
        if (epoll_fired_events_num == -1)
        {
            perror("epoll_wait");
            break;
        }
        //处理所有事件
        for (n = 0; n < epoll_fired_events_num; ++n)
        {
            if (fired_events[n].data.fd == listen_fd)
            {
                new_fd = accept(listen_fd, (struct sockaddr *)&their_addr, &len);
                if (new_fd < 0)
                {
                    perror("accept");
                    continue;
                }
                else
                    printf("有连接来自于： %s:%d， 分配的 socket 为:%d\n", inet_ntoa(their_addr.sin_addr), ntohs(their_addr.sin_port), new_fd);

                setnonblocking(new_fd);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = new_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &ev) < 0)
                {
                    fprintf(stderr, "把 socket '%d' 加入 epoll 失败！%s\n", new_fd, strerror(errno));
                    return -1;
                }
                curfds++;
            }
            else
            {
                ret = handle_message(fired_events[n].data.fd);
                if (ret < 1 && errno != 11)
                {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fired_events[n].data.fd, &ev);
                    curfds--;
                }
            }
        }
    }
    close(listen_fd);
    return 0;
}