#include <halos/mutex.h>
#include <halos/task.h>
#include <halos/interrupt.h>
#include <halos/assert.h>

//初始化互斥量
void mutex_init(mutex_t *mutex){
    mutex->value = false;//初始化为0省点时间
    list_init(&mutex->waiters);
}
//申请互斥量
void mutex_lock(mutex_t *mutex){
    //关闭中断
    bool intr = interrupt_disable();

    task_t *current = running_task();
    while (mutex->value == true)
    {
        //放到当前的互斥量队列中
        task_block(current, &mutex->waiters, TASK_BLOCKED);
    }
    assert(mutex->value == false);
    mutex->value++;//持有
    assert(mutex->value == true);
    //恢复中断
    set_interrupt_state(intr);
}
//释放互斥量
void mutex_unlock(mutex_t *mutex){
    //关闭中断
    bool intr = interrupt_disable();
    assert(mutex->value == true);
    mutex->value--;
    assert(mutex->value == false);

    //从等待队列中恢复一个执行
    if(!list_empty(&mutex->waiters)){
        task_t *task = element_entry(task_t, node, mutex->waiters.tail.prev);
        assert(task->magic == HALOS_MAGIC);
        task_unblock(task);
        task_yield();//保证不会饿死进程
    }

    //恢复中断
    set_interrupt_state(intr);
}

//初始化锁
void lock_init(lock_t *lock){
    lock->hodler = NULL;
    lock->repeat = 0;
    mutex_init(&lock->mutex);
}
//加锁
void lock_acquire(lock_t *lock){
    task_t *current = running_task();
    if(lock->hodler != current){
        //尝试获取互斥量
        mutex_lock(&lock->mutex);
        //成功后变更持有者
        lock->hodler = current;
        assert(lock->repeat == 0);
        lock->repeat = 1;
    }else{
        lock->repeat++;
    }
} 
//解锁
void lock_release (lock_t *lock){
    task_t *current = running_task();
    assert(lock->hodler == current);
    if (lock->repeat > 1)
    {
        lock->repeat--;
        return;
    }

    assert(lock->repeat == 1);
    lock->hodler = NULL;
    lock->repeat = 0;
    mutex_unlock(&lock->mutex);
}