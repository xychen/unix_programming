#ifndef __ANET_H
#define __ANET_H
#include <sys/types.h>

#define ANET_OK 0
#define ANET_ERR -1
#define ANET_ERR_LEN 256

int anetSetBlock(char *err, int fd, int non_block);
int anetNonBlock(char *err, int fd);
int anetTcpServer(char *err, int port, char *bindaddr, int backlog);

#endif
