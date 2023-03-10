#include <halos/interrupt.h>
#include <halos/syscall.h>
#include <halos/debug.h>
#include <halos/printk.h>
#include <halos/task.h>
#include <halos/stdio.h>
#include <halos/arena.h>
#include <halos/stdlib.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

void idle_thread(){
    set_interrupt_state(true);
    u32 counter = 0;
    while (true)
    {
        // LOGK("idle task...%d\n", counter++);
        asm volatile(
            "sti\n" //开中断
            "hlt\n" //关闭CPU，等待外中断的到来
        );
        yield(); //外中断来了后，放弃执行权
    }
}

static void user_init_thread(){
    u32 counter = 0;
    int status;
    while (true)
    {
        // pid_t pid = fork();
        // if (pid){
        //     printf("fork after parent %d %d %d...\n", pid, get_pid(), get_ppid());
        //     pid_t child = waitpid(pid, &status);
        //     printf("wait pid %d status %d %d...\n", child, status, time());
        // }else{
        //     printf("fork after child %d %d %d...\n", pid, get_pid(), get_ppid());
        //     // sleep(1000);
        //     exit(0);
        // }
        sleep(1000);
    }
}

void init_thread(){
    // set_interrupt_state(true);
    // while (true)
    // {
    //     printk("ini\n");
    // }
    char temp[100];//为了栈顶有足够的空间
    task_to_user_mode(user_init_thread);
}

void test_thread(){
    set_interrupt_state(true);
    while (true)
    {   
        test();
        sleep(10);
    }
}