/**
 * 设置非阻塞模式
 * 参考文章：http://zhoulifa.bokee.com/6063041.html
 * gcc -Wall simple-nonblock.c
 * nc -l 3000 < somefile.txt
 * ./a.out 127.0.0.1 3000 127.0.0.1 3100
 * 
 * */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAXBUF 10

int main(int argc, char **argv)
{
    int sockfd, ret, rcvtm = 0;
    struct sockaddr_in dest, mine;
    char buffer[MAXBUF + 1];

    if (argc != 5)
    {
        printf(
            "参数格式错误！正确用法如下：\n\t\t%s 对方IP地址 对方端口 本机IP地址 "
            "本机端口\n\t比如:\t%s 127.0.0.1 80\n此程序用来以本机固定的端口从某个 "
            "IP 地址的服务器某个端口接收最多 MAXBUF 个字节的消息",
            argv[0], argv[0]);
        exit(0);
    }

    /* 创建一个 socket 用于 tcp 通信 */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket");
        exit(errno);
    }

    /* 初始化服务器端（对方）的地址和端口信息 */
    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(atoi(argv[2]));
    if (inet_aton(argv[1], (struct in_addr *)&dest.sin_addr.s_addr) == 0)
    {
        perror(argv[1]);
        exit(errno);
    }

    /* 初始化自己的地址和端口信息 */
    bzero(&mine, sizeof(mine));
    mine.sin_family = AF_INET;
    mine.sin_port = htons(atoi(argv[4]));
    if (inet_aton(argv[3], (struct in_addr *)&mine.sin_addr.s_addr) == 0)
    {
        perror(argv[3]);
        exit(errno);
    }

    /* 把自己的 IP 地址信息和端口与 socket 绑定 */
    if (bind(sockfd, (struct sockaddr *)&mine, sizeof(struct sockaddr)) == -1)
    {
        perror(argv[3]);
        exit(errno);
    }

    /* 连接服务器 */
    if (connect(sockfd, (struct sockaddr *)&dest, sizeof(dest)) != 0)
    {
        perror("Connect ");
        exit(errno);
    }

    /* 设置 socket 属性为非阻塞方式 */
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1)
    {
        perror("fcntl");
        exit(errno);
    }

    /* 接收对方发过来的消息，每次最多接收 MAXBUF
   * 个字节，直到把对方发过来的所有消息接收完毕为止 */
    do
    {
    _retry:
        bzero(buffer, MAXBUF + 1);
        ret = recv(sockfd, buffer, MAXBUF, 0);
        if (ret > 0)
            printf("读到%d个字节，它们是:'%s'\n", ret, buffer);

        if (ret < 0)
        {
            if (errno == EAGAIN)
            {
                if (rcvtm)
                    break;
                else
                {
                    printf("数据还未到达！\n");
                    usleep(100000);
                    goto _retry;
                };
            };
            printf("接收出错了！\n");
            perror("recv");
        }
        rcvtm++;
    } while (ret == MAXBUF);

    /* 关闭连接 */
    close(sockfd);
    return 0;
}

/**
 * 1、非阻塞是什么？
网络通信有阻塞和非阻塞之分，例如对于接收数据的函数recv：在阻塞方式下，没有数据到达时，即接收不到数据时，程序会停在recv函数这里等待数据的到来；而在非阻塞方式下就不会等，如果没有数据可接收就立即返回-1表示接收失败。
2、什么是errno？
errno是Linux系统下保存当前状态的一个公共变量，当前程序运行时进行系统调用如果出错，则会设置errno为某个值以告诉用户出了什么错误。可以用printf("%d %s\n", errno, strerror(errno));得到具体信息。
3、什么是EAGAIN？
man recv
当recv系统调用返回这个值时表示recv读数据时，对方没有发送数据过来。
**/