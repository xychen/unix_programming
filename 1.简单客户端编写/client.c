/**
 * 简单客户端
 * 参考文章：http://zhoulifa.bokee.com/6062858.html
 * 启动服务端：echo "hello world" | nc -l 10000
 * 启动本客户端： ./a.out 127.0.0.1 10000
 * 运行结果：客户端打印输出 hello world 
 * */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAXBUF 1024

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in dest;
    char buffer[MAXBUF];

    if (argc != 3)
    {
        printf("参数格式错误！正确用法如下：\n\t\t%s IP 端口\n", argv[0]);
        exit(0);
    }

    //创建一个socket fd
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket create error");
        exit(errno);
    }

    //初始化服务端的ip和端口
    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    //htons() function converts the unsigned short integer hostshort from host byte order to network byte order
    dest.sin_port = htons(atoi(argv[2]));
    // ip地址转换成一个32位的网络序列ip
    if (inet_aton(argv[1], (struct in_addr *)&dest.sin_addr.s_addr) == 0)
    {
        perror(argv[1]);
        exit(errno);
    }

    //连接服务器
    if (connect(sockfd, (struct sockaddr *)&dest, sizeof(dest)) != 0)
    {
        perror("connect error");
        exit(errno);
    }

    //接受对方发过来的消息，最多接受MAXBUF个字节
    bzero(buffer, MAXBUF);
    recv(sockfd, buffer, sizeof(buffer), 0);
    printf("%s\n", buffer);

    //关闭连接
    close(sockfd);
    return 0;
}