#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
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

    struct sockaddr_in client_address;
    socklen_t len = sizeof(client_address);
    int connfd = accept(sockfd, (struct sockaddr *)&client_address, &len);
    if (connfd < 0)
    {
        printf("errno is: %d\n", errno);
    }
    else
    {
        int pipefd[2];
        ret = pipe(pipefd);
        assert(ret != -1);
        ret = splice(connfd, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        assert(ret != -1);
        ret = splice(pipefd[0], NULL, connfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        assert(ret != -1);
        close(connfd);
    }
    close(sockfd);
    return 0;
}