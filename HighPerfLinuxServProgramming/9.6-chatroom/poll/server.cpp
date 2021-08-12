#define _GNU_SOURCE 1
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <poll.h>

#define USER_LIMIT 5   //最大用户数
#define BUFFER_SIZE 64 //读缓冲区的大小
#define FD_LIMIT 65535 //文件描述符数量限制

struct client_data
{
    sockaddr_in address;
    char *write_buf;
    char buf[BUFFER_SIZE];
};

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

int main(int argc, char *argv[])
{
    if (argc <= 2)
    {
        printf("usage: %s ip_address port_number filename\n", argv[0]);
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    int ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    //用fd作为index，保证能放下所有fd
    client_data *users = new client_data[FD_LIMIT];

    //限制用户数量
    pollfd fds[USER_LIMIT + 1];

    int user_counter = 0;
    //初始化fds数组
    for (int i = 1; i <= USER_LIMIT; ++i)
    {
        fds[i].fd = -1;
        fds[i].events = 0;
    }
    fds[0].fd = listenfd;
    fds[0].events = POLLIN | POLLERR;
    fds[0].revents = 0;

    while (1)
    {
        ret = poll(fds, user_counter + 1, -1);
        if (ret < 0)
        {
            printf("poll failure\n");
            break;
        }

        for (int i = 0; i < user_counter + 1; ++i)
        {
            if ((fds[i].fd == listenfd) && (fds[i].revents & POLLIN))       //新连接处理
            {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);
                if (connfd < 0)
                {
                    printf("errno is: %d\n", errno);
                    continue;
                }
                //请求太多，关闭新连接
                if (user_counter >= USER_LIMIT)
                {
                    const char *info = "too many users\n";
                    printf("%s", info);
                    send(connfd, info, strlen(info), 0);
                    close(connfd);
                    continue;
                }
                user_counter++;
                users[connfd].address = client_address;
                setnonblocking(connfd);
                fds[user_counter].fd = connfd;
                //关注的时间类型
                fds[user_counter].events = POLLIN | POLLRDHUP | POLLERR;
                fds[user_counter].revents = 0;
                printf("comes a new user, now have % users\n", user_counter);
            }
            else if (fds[i].revents & POLLERR)      //该连接发生错误事件
            {
                printf("get an error from %d\n", fds[i].fd);
                char errors[100];
                memset(errors, '\0', 100);
                socklen_t length = sizeof(errors);
                if (getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, &length) < 0)
                {
                    printf("get socket option failed\n");
                }
                continue;
            }
            else if (fds[i].revents & POLLRDHUP)    //该连接发生关闭事件
            {
                //客户端关闭连接 (把已连接的最后一个挪到这个位置)
                users[fds[i].fd] = users[fds[user_counter].fd];
                //关闭连接
                close(fds[i].fd);
                fds[i] = fds[user_counter];
                
                i--;    //（因为上边把已连接的最后一个挪到了当前位置，所以让当前位置的fd被再遍历到一次，i--后，for循环中还要i++）
                user_counter--;     //已连接的总数-1
                printf("a client left\n");  
            }
            else if (fds[i].revents & POLLIN)       //该连接有读事件发生
            {
                //有读事件发生
                int connfd = fds[i].fd;
                memset(users[connfd].buf, '\0', BUFFER_SIZE);
                ret = recv(connfd, users[connfd].buf, BUFFER_SIZE - 1, 0);
                printf("get %d bytes of client data %s from %d\n", ret, users[connfd].buf, connfd);
                if (ret < 0)
                {
                    //读操作出错，则关闭连接
                    if (errno != EAGAIN)
                    {
                        close(connfd);
                        users[fds[i].fd] = users[fds[user_counter].fd];
                        fds[i] = fds[user_counter];
                        i--;
                        user_counter--;
                    }
                }
                else if (ret == 0)
                {
                    //读到空
                }
                else
                {
                    //正常数据，通知其他socket准备写数据（群发消息）
                    for (int j = 0; j <= user_counter; j++)
                    {
                        if (fds[j].fd == connfd)        //不用给自己发消息了
                        {
                            continue;
                        }
                        // 暂时移除对读事件的关注，添加对写事件的关注
                        fds[j].events |= ~POLLIN;
                        fds[j].events |= POLLOUT;

                        //把读到的buf内容直接复制给写buf
                        //@todo: 多个读事件发生时，最后一个会覆盖前边的吧？ 
                        users[fds[j].fd].write_buf = users[connfd].buf;
                    }
                }
            }
            else if (fds[i].revents & POLLOUT)          //该连接有写事件发生(只要socket可写，就能直接触发写事件)
            {
                int connfd = fds[i].fd;
                //没有要写的数据
                if (!users[connfd].write_buf)
                {
                    continue;
                }
                //发送数据
                ret = send(connfd, users[connfd].write_buf, strlen(users[connfd].write_buf), 0);
                //清空write_buf
                users[connfd].write_buf = NULL;
                //写完数据后需要重新注册fds[i]上的读事件
                fds[i].events |= ~POLLOUT;
                fds[i].events |= POLLIN;
            }
        }
    }
    delete[] users;
    close(listenfd);
    return 0;
}
