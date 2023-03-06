#include <halos/interrupt.h>
#include <halos/assert.h>
#include <halos/debug.h>
#include <halos/syscall.h>
#include <halos/list.h>
#include <halos/task.h>
#include <halos/console.h>
#include <halos/memory.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)
//系统调用数
#define SYSCALL_SIZE 256

handler_t syscall_table[SYSCALL_SIZE];

void syscall_check(u32 nr){
    if(nr > SYSCALL_SIZE){
        panic("Syscall nr Error");
    }
}

static void sys_defult(){
    panic("Syscall Not Implemented");
}

static u32 sys_test(){
    return 255;
}

int32 sys_write(fd_t fd, char *buf, int len){
    if(fd == stdout || fd == stderr){
        return console_write(buf, len);
    }
    panic("write!!!");
    return 0;
} 

extern time_t sys_time();
void syscall_init(){
    for (size_t i = 0; i < SYSCALL_SIZE; i++)
    {
        syscall_table[i] = sys_defult;
    }
    syscall_table[SYS_NR_TEST] = sys_test;
    syscall_table[SYS_NR_EXIT] = task_exit;
    syscall_table[SYS_NR_FORK] = task_fork;
    syscall_table[SYS_NR_WAITPID] = task_waitpid;
    syscall_table[SYS_NR_TIME] = sys_time;
    syscall_table[SYS_NR_SLEEP] = task_sleep;
    syscall_table[SYS_NR_YIELD] = task_yield;
    syscall_table[SYS_NR_GETPID] = sys_getpid;
    syscall_table[SYS_NR_GETPPID] = sys_getppid;
    syscall_table[SYS_NR_BRK] = sys_brk;
    syscall_table[SYS_NR_WRITE] = sys_write;
}