
## 定时器原理
- alarm系统调用，会产生SIGALRM信号

```c
unsigned int alarm (unsigned int __seconds)
```

- 信号处理函数会将信号写入 pipefd中

```c
send(pipefd[1], (char *)&msg, 1, 0);
```

- 主流程中通过epoll_wait监听到pipefd有可读时间发生，读取信号值，并做相应处理

- 主流程中读到信号值，开始检查哪个连接过期了，将过期的连接关闭调

- 基于排序链表的定时器，效率偏低，插入、删除操作的时间复杂度为O(n)