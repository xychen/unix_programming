#ifndef LST_TIMER
#define LST_TIMER

#include <time.h>
#include <netinet/in.h>
#include <stdio.h>

#define BUFFER_SIZE 64
class util_timer;

struct client_data
{
    struct sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    util_timer *timer;
};

class util_timer
{
public:
    util_timer() : prev(NULL), next(NULL) {}

public:
    time_t expire;                  //任务的超时时间，这里使用绝对时间
    void (*cb_func)(client_data *); //任务回调函数
    client_data *user_data;         //回调函数处理的客户数据，由定时器的执行者传递给回调函数
    util_timer *prev;               //指向前一个定时器
    util_timer *next;               //指向下一个定时器
};

class sort_timer_lst
{
private:
    util_timer *head;
    util_timer *tail;

public:
    sort_timer_lst() : head(NULL), tail(NULL) {}
    //链表被销毁时，删除其中所有的定时器
    ~sort_timer_lst()
    {
        util_timer *tmp = head;
        while (tmp)
        {
            head = tmp->next;
            delete tmp;
            tmp = head;
        }
    }

    //将目标定时器timer添加到链表中
    void add_timer(util_timer *timer)
    {
        if (!timer)
        {
            return;
        }
        if (!head)
        {
            head = tail = timer;
            return;
        }

        //如果目标定时器的超时时间小于当前链表中所有定时器的超时时间，则把改定时器插入链表头部，作为链表新的头结点。
        //否则需要调用重载函数 add_timer(util_timer *timer, util_timer *lst_head) ,把它插入链表中合适的位置，以保证链表的升序特性
        if (timer->expire < head->expire)
        {
            timer->next = head;
            head->prev = timer;
            head = timer;
            return;
        }
        add_timer(timer, head);
    }

    //当某个定时任务发生变化时，调整对应的定时器在链表中的位置
    //这个函数只考虑被调整的定时器的超时时间延长的情况，即该定时器需要往链表的尾部移动
    void adjust_timer(util_timer *timer)
    {
        if (!timer)
        {
            return;
        }
        util_timer *tmp = timer->next;
        //如果被调整的目标定时器处在链表尾部，或者该定时器新的超时值仍然小于其下一个定时器的超时值，则不用调整
        if (!tmp || (timer->expire < tmp->expire))
        {
            return;
        }
        //如果目标定时器是链表的头结点，则将该定时器从链表中取出并重新插入链表
        if (timer == head)
        {
            head = head->next;
            head->prev = NULL;
            timer->next = NULL;
            add_timer(timer, head);
        }
        else
        {
            //如果目标定时器不是链表的头结点，则将该定时器从链表中取出，然后插入其原来所在位置之后的部分链表中
            timer->prev->next = timer->next;
            timer->next->prev = timer->prev;
            add_timer(timer, timer->next);
        }
    }

    void del_timer(util_timer *timer)
    {
        if (!timer)
        {
            return;
        }
        //只有一个定时器，即目标定时器
        if ((timer == head) && (timer == tail))
        {
            delete timer;
            head = NULL;
            tail = NULL;
            return;
        }
        //如果链表中至少有2个定时器，且目标定时器是链表的头结点，则将链表的头结点重置为原头节点的下一个节点，然后删除目标定时器
        if (timer == head)
        {
            head = head->next;
            head->prev = NULL;
            delete timer;
            return;
        }
        //如果链表中至少有2个定时器，且目标定时器是链表的头结点，则将链表的尾结点重置为原尾结点的前一个界定，然后删除目标定时器
        if (timer == tail)
        {
            tail = tail->prev;
            tail->next = NULL;
            delete timer;
            return;
        }

        //如果目标定时器位于链表的中间，则把它前后的定时器串联起来，然后删除目标定时器
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        delete timer;
    }

    void tick()
    {
        if (!head)
        {
            return;
        }
        printf("timer tick\n");
        //获取系统当前时间
        time_t cur = time(NULL);
        util_timer *tmp = head;
        //核心逻辑：从头结点开始依次处理每个定时器，知道遇到一个尚未到期的定时器
        while (tmp)
        {
            //比较定时器是否到期
            if (cur < tmp->expire)
            {
                break;
            }
            //到期的定时任务，调用回调函数
            tmp->cb_func(tmp->user_data);
            //执行完定时器的定时任务之后，就将它从链表中删除，并充值链表的头结点
            head = tmp->next;
            if (head)
            {
                head->prev = NULL;
            }
            delete tmp;
            tmp = head;
        }
    }

private:
    void add_timer(util_timer *timer, util_timer *lst_head)
    {
        util_timer *prev = lst_head;
        util_timer *tmp = prev->next;
        //遍历lst_head节点之后的部分链表，知道找到一个超时时间大于目标定时器的超时时间的节点，并将目标定时器插入该节点之前
        while (tmp)
        {
            if (timer->expire < tmp->expire)
            {
                prev->next = timer;
                timer->next = tmp;
                tmp->prev = timer;
                timer->prev = prev;
                break;
            }
            prev = tmp;
            tmp = tmp->next;
        }
        //如果遍历完了也没找到比它大的节点，则是尾结点
        if (!tmp)
        {
            prev->next = timer;
            timer->prev = prev;
            timer->next = NULL;
            tail = timer;
        }
    }
};

#endif