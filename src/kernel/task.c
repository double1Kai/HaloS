#include <halos/task.h>
#include <halos/printk.h>
#include <halos/debug.h>

#define PAGE_SIZE 0x1000

extern void task_switch();

task_t *a = (task_t *)0x1000;
task_t *b = (task_t *)0x2000;

//获取当前这一页的起始位置，将esp后三位抹去
task_t *running_task(){
    asm volatile(
        "movl %esp, %eax\n"
        "andl $0xfffff000, %eax\n"
    );
}

void schedule(){
    task_t *current = running_task();
    task_t *next = current == a ? b : a;
    task_switch(next);//切换任务
}

u32 _ofp thread_a(){
    asm volatile("sti");
    while(true)
            printk("A");
}

u32 _ofp thread_b(){
    asm volatile("sti");
    while(true)
            printk("B");
}

static void task_create(task_t *task, target_t target){
    u32 stack = (u32)task + PAGE_SIZE;//栈顶从上往下
    stack -= sizeof(task_frame_t);
    task_frame_t *frame = (task_frame_t *)stack;
    frame->ebx = 0x11111111;
    frame->esi = 0x22222222;
    frame->edi = 0x33333333;
    frame->ebp = 0x44444444;
    frame->eip = (void *)target;//执行任务的入口

    task->stack = (u32 *)stack;
    }



void task_init(){
    task_create(a, thread_a);
    task_create(b, thread_b);
    schedule();
}