#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

static const char *status_line[2] = {"200 OK", "500 Internal server error"};

int main(int argc, char *argv[])
{
    if (argc <= 3)
    {
        printf("usage: %s ip_address port_number filename\n", basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);

    //目标文件作为程序的第3个参数传入
    const char *file_name = argv[3];

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int ret = bind(sock, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, 5);
    assert(ret != -1);

    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock, (struct sockaddr *)&client, &client_addrlength);

    if (connfd < 0)
    {
        printf("errno is: %d\n", errno);
        close(sock);
        return 0;
    }
    //用于保存http应答的状态行、头部字段和一个空行的缓存区
    char header_buf[BUFFER_SIZE];
    memset(header_buf, '\0', BUFFER_SIZE);

    //用于存放目标文件内容的应用程序缓存
    char *file_buf;

    //用于获取目标文件的属性，比如是否为目录，文件大小等
    struct stat file_stat;

    //记录目标文件是否为有效文件
    int valid = 1;

    //缓存区header_buf目前已经使用了多少字节的空间
    int len = 0;

    if (stat(file_name, &file_stat) < 0) //目标文件不存在
    {
        valid = 0;
    }
    else
    {
        if (S_ISDIR(file_stat.st_mode)) //目标文件是一个目录
        {
            valid = 0;
        }
        else if (file_stat.st_mode & S_IROTH) //当前用户有读取目标文件的权限
        {
            //动态分配缓存区file_buf, 并制定其大小为目标文件的大小加1（file_stat.st_size + 1）,然后将目标文件读入缓存区file_buf中
            int fd = open(file_name, O_RDONLY);
            // file_buf = new char[file_stat.st_size + 1];
            file_buf = (char *)malloc(sizeof(char) * (file_stat.st_size + 1));
            memset(file_buf, '\0', file_stat.st_size + 1);
            if (read(fd, file_buf, file_stat.st_size) < 0)
            {
                valid = 0;
            }
        }
        else
        {
            valid = 0;
        }
    }

    //目标文件无效，则通知客户端服务器发生了“内部错误”
    if (!valid)
    {
        ret = snprintf(header_buf, BUFFER_SIZE - 1, "%s %s\r\n", "HTTP/1.1", status_line[1]);
        len += ret;
        ret = snprintf(header_buf + len, BUFFER_SIZE - 1 - len, "%s", "\r\n");
        send(connfd, header_buf, strlen(header_buf), 0);
    }
    else //目标文件有效，正常的HTTP应答
    {
        //添加状态行
        ret = snprintf(header_buf, BUFFER_SIZE - 1, "%s %s\r\n", "HTTP/1.1", status_line[0]);
        len += ret;
        //添加Content-Length
        ret = snprintf(header_buf + len, BUFFER_SIZE - 1 - len, "Content-Length: %d\r\n", file_stat.st_size);
        len += ret;
        //添加空行
        ret = snprintf(header_buf + len, BUFFER_SIZE - 1 - len, "%s", "\r\n");

        //利用writev将header_buf和file_buf的内容一并写出
        struct iovec iv[2];
        iv[0].iov_base = header_buf;
        iv[0].iov_len = strlen(header_buf);
        iv[1].iov_base = file_buf;
        iv[1].iov_len = file_stat.st_size;
        ret = writev(connfd, iv, 2);
    }
    //关闭连接
    close(connfd);
    //释放空间
    free(file_buf);

    close(sock);
    return 0;
}