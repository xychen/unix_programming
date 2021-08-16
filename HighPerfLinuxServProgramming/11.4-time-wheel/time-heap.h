#ifndef MIN_HEAP
#define MIN_HEAP

#include <iostream>
#include <netinet/in.h>
#include <time.h>
using std::exception;

#define BUFFER_SIZE 64

class heap_timer; //前项声明
struct client_data
{
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    heap_timer *timer;
};

//定时器类
class heap_timer
{
public:
    heap_timer(int delay)
    {
        expire = time(NULL) + delay;
    }

public:
    time_t expire;                  //定时器生效的绝对时间
    void (*cb_func)(client_data *); //定时器的回调含糊
    client_data *user_data;         //用户数据
};

//时间堆类
class time_heap
{
public:
    //构造函数，初始化一个大小为cap的空堆
    time_heap(int cap) throw(std::exception) : capacity(cap), cur_size(0)
    {
        array = new heap_timer *[capacity]; //创建堆数组
        if (!array)
        {
            throw std::exception();
        }
        for (int i = 0; i < capacity; i++)
        {
            array[i] = NULL;
        }
    }

    time_heap(heap_timer **init_array, int size, int capacity) throw(std::exception) : cur_size(size), capacity(capacity)
    {
        if (capacity < size)
        {
            throw std::exception();
        }
        array = new heap_timer *[capacity]; //创建堆数组
        if (!array)
        {
            throw std::exception();
        }

        for (int i = 0; i < capacity; i++)
        {
            array[i] = NULL;
        }

        if (size != 0)
        {
            for (int i = 0; i < size; i++)
            {
                array[i] = init_array[i];
            }

            for (int i = (cur_size - 1) / 2; i >= 0; i--)
            {
                //调整堆
                percolate_down(i);
            }
        }
    }

    ~time_heap()
    {
        for (int i = 0; i < cur_size; i++)
        {
            delete array[i];
        }
        delete[] array;
    }

public:
    void add_timer(heap_timer *timer) throw(std::exception)
    {
        if (!timer)
        {
            return;
        }
        if (cur_size >= capacity) //如果当前堆数组容量不够，则将其扩大1倍
        {
            resize();
        }
        //新插入一个元素，当前堆大小加1，hole是新建空穴的位置
        int hole = cur_size++;
        int parent = 0;

        for (; hole > 0; hole = parent)
        {
            parent = (hole - 1) / 2;
            if (array[parent]->expire <= timer->expire)
            {
                break;
            }
            //调整
            array[hole] = array[parent];
        }
        array[hole] = timer;
    }

    void del_timer(heap_timer *timer)
    {
        if (!timer)
        {
            return;
        }
        //不是真正删除，会使堆数组膨胀
        timer->cb_func = NULL;
    }

    heap_timer *top() const
    {
        if (empty())
        {
            return NULL;
        }
        return array[0];
    }

    //删除堆顶的定时器
    void pop_timer()
    {
        if (empty())
        {
            return;
        }
        if (array[0])
        {
            delete array[0];
            array[0] = array[--cur_size];
            //从上到下调整堆
            percolate_down(0);
        }
    }

    void tick()
    {
        heap_timer *tmp = array[0];
        time_t cur = time(NULL);
        while (!empty())
        {
            if (!tmp)
            {
                break;
            }
            if (tmp->expire > cur)
            {
                break;
            }
            if (array[0]->cb_func)
            {
                array[0]->cb_func(array[0]->user_data);
            }
            //将堆顶元素删除，同时生成新的堆顶定时器
            pop_timer();
            tmp = array[0];
        }
    }

    bool empty() const { return cur_size == 0; };

private:
    //从上到下调整堆
    void percolate_down(int hole)
    {
        heap_timer *temp = array[hole];
        int child = 0;
        for (; ((hole * 2 + 1) <= (cur_size - 1)); hole = child)
        {
            child = hole * 2 + 1;
            //找到比较小的孩子节点
            if ((child < (cur_size - 1)) && (array[child + 1]->expire < array[child]->expire))
            {
                child++;
            }

            if (array[child]->expire < temp->expire)
            {
                array[hole] = array[child];
            }
            else
            {
                break;
            }
        }
        array[hole] = temp;
    }

    void resize() throw(std::exception)
    {
        heap_timer **temp = new heap_timer *[2 * capacity];
        for (int i = 0; i < 2 * capacity; i++)
        {
            temp[i] = NULL;
        }
        if (!temp)
        {
            throw std::exception();
        }
        capacity = 2 * capacity;
        for (int i = 0; i < cur_size; i++)
        {
            temp[i] = array[i];
        }
        delete[] array;
        array = temp;
    }

private:
    heap_timer **array; //堆数组
    int capacity;       //堆大小
    int cur_size;       //当前堆元素个数
};

#endif