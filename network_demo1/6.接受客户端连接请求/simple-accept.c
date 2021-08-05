/**
 * 接受客户端连接请求
 * 参考文章：http://zhoulifa.bokee.com/6064418.html
 * 编译程序用下列命令：gcc -Wall simple-accept.c
 * 运行程序用如下命令：./a.out 7838 1 127.0.0.1
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


int main(int argc, char **argv)
{
    int sockfd, new_fd;
    socklen_t len;
    struct sockaddr_in my_addr, their_addr;
    unsigned int myport, lisnum;

    if (argv[1])
        myport = atoi(argv[1]);
    else
        myport = 7838;

    if (argv[2])
        lisnum = atoi(argv[2]);
    else
        lisnum = 2;

    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }
    else
        printf("socket created\n");

    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = PF_INET;
    my_addr.sin_port = htons(myport);
    if (argv[3])
        my_addr.sin_addr.s_addr = inet_addr(argv[3]);
    else
        my_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("bind");
        exit(1);
    }
    else
        printf("binded\n");

    if (listen(sockfd, lisnum) == -1)
    {
        perror("listen");
        exit(1);
    }
    else
        printf("begin listen\n");

    while (1)
    {
        len = sizeof(struct sockaddr);
        if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &len)) == -1)
        {
            perror("accept");
            exit(errno);
        }
        else
            printf("server: got connection from %s, port %d, socket %d\n", inet_ntoa(their_addr.sin_addr), ntohs(their_addr.sin_port), new_fd);
    }

    close(sockfd);
    return 0;
}