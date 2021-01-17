/**
 * 绑定客户端ip和端口
 * 参考文章：http://zhoulifa.bokee.com/6062924.html
 * 启动服务端：echo "hello world" | nc -l 10000
 * 启动本客户端： ./a.out 127.0.0.1 10000 127.0.0.1 10001
 * 运行结果：客户端打印输出 hello world 
 * */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAXBUF 1024

int main(int argc, char **argv)
{
    int sockfd;
    //sockaddr_in: Structures for handling internet addresses
    struct sockaddr_in dest, mine;
    char buffer[MAXBUF];

    if (argc != 5)
    {
        printf("参数格式错误！正确用法如下:\n\t %s 对方ip 对方端口 本机ip 本机端口", argv[0]);
        exit(0);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket create error");
        exit(errno);
    }

    //初始化服务端(对方)的地址和端口信息
    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(atoi(argv[2]));
    if (inet_aton(argv[1], (struct in_addr *)&dest.sin_addr.s_addr) == 0)
    {
        perror("对方ip错误");
        exit(errno);
    }

    //初始化自己的地址和端口信息
    bzero(&mine, sizeof(mine));
    mine.sin_family = AF_INET;
    mine.sin_port = htons(atoi(argv[4]));
    if (inet_aton(argv[3], (struct in_addr *)&mine.sin_addr.s_addr) == 0)
    {
        perror(argv[3]);
        exit(errno);
    }

    //把自己的ip地址信息和端口与socket绑定
    if (bind(sockfd, (struct sockaddr *)&mine, sizeof(struct sockaddr)) == -1)
    {
        perror("bind 失败");
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
    sleep(5);

    //关闭连接
    close(sockfd);

    return 0;
}