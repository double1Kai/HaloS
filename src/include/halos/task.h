#ifndef HALOS_TASK_H
#define HALOS_TASK_H

#include <halos/types.h>
#include <halos/bitmap.h>
#include <halos/list.h>

#define KERNEL_USER 0
#define NORMAL_USER 1

#define TASK_NAME_LEN 16

typedef void target_t();//入口地址

typedef enum task_state_t
{
    TASK_INIT,      //初始化
    TASK_RUNNING,   //执行
    TASK_READY,     //就绪
    TASK_BLOCKED,   //阻塞
    TASK_SLEEPING,  //休眠
    TASK_WAITTING,  //等待
    TASK_DEAD,      //死亡
} task_state_t;

typedef struct task_t
{
    u32 *stack;         //内核栈
    list_node_t node;   //任务阻塞节点
    task_state_t state; //状态
    u32 priority;       //优先级
    int ticks;          //剩余时间片
    u32 jiffies;        //上次执行时的全局时间片
    char name[TASK_NAME_LEN];//任务名
    u32 uid;            //用户id
    pid_t pid;          //任务id
    pid_t ppid;         //任务父进程id
    u32 pde;            //页目录物理地址，每个进程一张
    struct bitmap_t *vamp;//进程虚拟内存位图
    u32 brk;            //进程堆内存最高地址
    int status;         //进程特殊状态
    pid_t waitpid;      //进程等待的pid
    struct inode_t *ipwd;//进程当前目录的inode，program work directory
    struct inode_t *iroot; //进程根目录的inode
    u32 magic;          //魔数，用于检测栈溢出
} task_t;

typedef struct task_frame_t
{
    u32 edi;
    u32 esi;
    u32 ebx;
    u32 ebp;
    void (*eip)(void);
} task_frame_t;

// 中断帧，中断发生时，用户态进内核态时会压入栈顶
typedef struct intr_frame_t
{
    u32 vector;

    u32 edi;
    u32 esi;
    u32 ebp;
    // 虽然 pushad 把 esp 也压入，但 esp 是不断变化的，所以会被 popad 忽略
    u32 esp_dummy;

    u32 ebx;
    u32 edx;
    u32 ecx;
    u32 eax;

    u32 gs;
    u32 fs;
    u32 es;
    u32 ds;

    u32 vector0;

    u32 error;

    u32 eip;
    u32 cs;
    u32 eflags;
    u32 esp;
    u32 ss;
} intr_frame_t;

task_t *running_task();
void schedule();

void task_yield();
void task_exit();
pid_t task_fork();
pid_t task_waitpid(pid_t pid, int32* status);
void task_block(task_t *task, list_t *blist, task_state_t state);
void task_unblock(task_t *task);
void task_sleep(u32 ms);
void task_wakeup();
void task_to_user_mode(target_t target);
pid_t sys_getpid();
pid_t sys_getppid();

#endif