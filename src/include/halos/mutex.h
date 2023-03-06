#ifndef HALOS_MUTEX_H
#define HALOS_MUTEX_H
#include <halos/types.h>
#include <halos/list.h>
#include <halos/task.h>

typedef struct mutex_t
{
    bool value; //信号量
    list_t waiters; //等待队列
} mutex_t;

void mutex_init(mutex_t *mutex);//初始化互斥量
void mutex_lock(mutex_t *mutex);//申请互斥量
void mutex_unlock(mutex_t *mutex);//释放互斥量

//互斥锁
typedef struct lock_t
{
    task_t *hodler; //持有者
    mutex_t mutex; //互斥量
    u32 repeat;     //重入次数
} lock_t;

void lock_init(lock_t *lock);//初始化锁
void lock_acquire(lock_t *lock);   //加锁
void lock_release(lock_t *lock); //解锁


#endif