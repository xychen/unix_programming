#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

struct msg
{
    struct msg *m_next;
};

struct msg *workq;

pthread_cond_t qready = PTHREAD_COND_INITIALIZER;
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;

void maketimeout(struct timespec *tsp, long minutes)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    tsp->tv_sec = now.tv_sec;
    tsp->tv_nsec = now.tv_usec * 1000;
    tsp->tv_sec += minutes * 60;
}

//处理消息
void process_msg(void)
{
    struct msg *mp;

    for (;;)
    {
        pthread_mutex_lock(&qlock);
        //如果队列为空，就等待
        while (workq == NULL)
        {
            //将锁住的互斥量传给函数，函数【自动把调用线程放到等待条件的线程列表】上，对互斥量解锁
            //这就关闭了条件检查和线程进入休眠状态等待条件改变这2个操作之间的时间通道
            //pthread_cond_wait返回时，互斥量再次被锁住
            pthread_cond_wait(&qready, &qlock);
            mp = workq;
            workq = mp->m_next;
            pthread_mutex_unlock(&qlock);
        }
    }
}

//生产消息
void enqueue_msg(struct msg *mp)
{
    pthread_mutex_lock(&qlock);
    mp->m_next = workq;
    workq = mp;
    pthread_mutex_unlock(&qlock);
    //生产消息后，进行通知
    pthread_cond_signal(&qready);
}
