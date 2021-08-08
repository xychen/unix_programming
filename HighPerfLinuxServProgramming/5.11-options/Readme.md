
## option函数

```c
#include <sys/socket.h>
int getsockopt(int sockfd, int level, int option_name, void *option_value, socklen_t *restrict option_len)
int setsockopt(int sockfd, int level, int option_name, const void *option_value, socklen_t option_len)
```

## SO_RCVBUF和SO_SNDBUF选项

- SO_RCVBUF和SO_SNDBUF分别表示TCP接受缓冲区和发送缓冲区的大小。不过，当我们用setsockopt设置大小时，系统会将其值加倍，并且不小于某个最小值。
- 系统这样做的目的是确保TCP链接拥有足够的空闲缓冲区来处理拥塞（比如快速重传算法就期望TCP接受缓冲区能至少容纳4个大小为SMSS的TCP报文段）
- 可以修改/proc/sys/net/ipv4/tcp_rmem 和 /proc/sys/net/ipv4/tcp_wmem 来强制设置这2个参数
- 抓包结果：

## 问题  
@todo: tcp_tw_recyle 和 SO_REUSEDDR的区别？？