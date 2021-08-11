
- 由于pthread库不是标准linux库，编译时需要指定

```
gcc epoll-oneshot.cpp -lpthread -o bin/epoll-oneshot
```