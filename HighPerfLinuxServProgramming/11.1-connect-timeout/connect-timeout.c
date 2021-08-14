

#include <stdio.h>
#include <libgen.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <strings.h>
#include <assert.h>
#include <errno.h>
#include <sys/time.h>

int timeout_connect(const char *ip, int port, int time)
{
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    //通过选项SO_RCVTIMEO 和 SO_SNDTIMEO所设置的超时时间的类型是timeval
    struct timeval timeout;
    timeout.tv_sec = time;
    timeout.tv_usec = 0;

    socklen_t len = sizeof(timeout);

    ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, len);
    assert(ret != -1);

    ret = connect(sockfd, (struct sockaddr *)&address, sizeof(address));
    if (ret == -1)
    {
        if (errno == EINPROGRESS)
        {
            printf("error occur when connecting to server\n");
            return -1;
        }
    }
    return sockfd;
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

    int sockfd = timeout_connect(ip, port, 1);
    if(sockfd < 0)
    {
        return 1;
    }
    printf("connect success - %d\n", sockfd);
    return 0;
}