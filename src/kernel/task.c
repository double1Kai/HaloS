#include <halos/task.h>
#include <halos/printk.h>
#include <halos/debug.h>
#include <halos/memory.h>
#include <halos/assert.h>
#include <halos/interrupt.h>
#include <halos/string.h>
#include <halos/bitmap.h>
#include <halos/syscall.h>
#include <halos/global.h>
#include <halos/arena.h>
#include <halos/types.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define NR_TASKS 64

extern u32 volatile jiffies;
extern u32 jiffy;

extern bitmap_t kernel_map;
extern tss_t tss;

extern void task_switch(task_t *next);

//任务数量
#define NR_TASKS 64
//所有任务的数组
static task_t *task_table[NR_TASKS];

//任务默认阻塞链表
static list_t block_list;
//任务默认睡眠链表
static list_t sleep_list;

//空闲进程
task_t *idle_task;

//从taks_table中获取一个空置的任务
static task_t *get_free_task(){
    for(size_t i = 0; i < NR_TASKS; i++){
        if(task_table[i] == NULL){
            //如果空闲，则分配一页内核内存，并返回
            task_t *task = (task_t *)alloc_kpage(1);
            memset(task, 0, PAGE_SIZE);
            task->pid = i;
            task_table[i] = task;
            return task;
        }
    }
    panic("No More Task!!!\n");
}

//获取进程id
pid_t sys_getpid(){
    task_t *task = running_task();
    return task->pid;
}
//获取父进程id
pid_t sys_getppid(){
    task_t *task = running_task();
    return task->ppid;
}

//从任务数组中查找某一种状态的任务，自己除外
static task_t *task_search(task_state_t state){
    assert(!get_interruput_state());//原子操作，不可中断
    task_t *task = NULL;
    task_t *current = running_task();
    for(size_t i = 0; i < NR_TASKS; i++){
        task_t *ptr = task_table[i];
        if(ptr == NULL){
            continue;
        }
        if(ptr->state != state){
            continue;
        }
        if(ptr == current){
            continue;
        }
        //时间片短的优先级更高，时间片相同的话，最晚执行的那个优先级更高
        if(task == NULL || task->ticks < ptr->ticks || ptr->jiffies < task->jiffies){
            task = ptr;
        }
    }
    //如果找不到READY的进程，则执行空闲进程
    if(task ==  NULL && state == TASK_READY){
        task = idle_task;
    }
    return task;
}

void task_yield(){
    schedule();
}

//任务阻塞
void task_block(task_t *task, list_t *blist, task_state_t state){
    assert(!get_interruput_state());
    assert(task->node.next == NULL);
    assert(task->node.prev == NULL);
    if(blist == NULL){
        blist = &block_list;
    }
    list_push(blist, &task->node);
    assert(state != TASK_READY && state != TASK_RUNNING);
    task->state = state;
    task_t *current = running_task();
    if(current == task){
        schedule();
    }
}

void task_unblock(task_t *task){
    assert(!get_interruput_state());
    list_remove(&task->node);

    assert(task->node.prev == NULL);
    assert(task->node.next == NULL);

    task->state = TASK_READY;
}

void task_sleep(u32 ms){
    assert(!get_interruput_state());
    u32 ticks = ms / jiffy;//睡眠多少个时间片
    ticks = ticks > 0 ? ticks : 1;//至少一个时间片

    //设置时间片为全局时间片+睡眠时间片
    task_t *current = running_task();
    current->ticks = jiffies + ticks;

    //在睡眠链表中按休眠时间由低到高插入
    list_insert_sort(&sleep_list, &current->node, element_node_offset(task_t, node, ticks));

    //改一下状态
    current->state = TASK_SLEEPING;
    //调度
    schedule();
}

void task_wakeup(){
    assert(!get_interruput_state());
    //从睡眠链表中找到ticks小于jiffies的任务，恢复执行
    list_t *list = &sleep_list;
    for(list_node_t *ptr = list->head.next; ptr != &list->tail;){
        task_t *task = element_entry(task_t, node, ptr);
        if(task->ticks > jiffies){
            break;
        }
        ptr = ptr->next;//unblock之后就没了，所以得放这里
        task->ticks = 0;
        //ublock函数会将指针清空
        task_unblock(task);
    }
}

//激活任务
void task_activate(task_t *task){
    assert(task->magic == HALOS_MAGIC);
    //页表不对应先设置一下
    if(task->pde != get_cr3()){
        set_cr3(task->pde);
    }
    //如果不是内核态，则把esp0布置为内核态的栈顶
    if(task->uid != KERNEL_USER){
        tss.esp0 = (u32)task +PAGE_SIZE;
    } 
}

//获取当前这一页的起始位置，将esp后三位抹去
task_t *running_task(){
    asm volatile(
        "movl %esp, %eax\n"
        "andl $0xfffff000, %eax\n"
    );
}

void schedule(){
    assert(!get_interruput_state());//不可中断

    task_t *current = running_task();
    task_t *next = task_search(TASK_READY);//找一个就绪态的任务

    assert(next != NULL);
    assert(next->magic  == HALOS_MAGIC);

    if(current->state == TASK_RUNNING){
        current->state = TASK_READY;
    }

    if(current->ticks == 0){
        current->ticks = current->priority;
    }

    next->state = TASK_RUNNING;
    if(next == current){
        return;
    }
    task_activate(next);
    task_switch(next);//切换任务
}

static task_t *task_create(target_t target, const char *name, u32 priority, u32 uid){
    task_t *task = get_free_task();

    u32 stack = (u32)task + PAGE_SIZE;//栈顶从上往下，栈放在页末尾

    stack -= sizeof(task_frame_t);
    task_frame_t *frame = (task_frame_t *)stack;
    frame->ebx = 0x11111111;
    frame->esi = 0x22222222;
    frame->edi = 0x33333333;
    frame->ebp = 0x44444444;
    frame->eip = (void *)target;//执行任务的入口

    strcpy((char *)task->name, name);

    //PCB放在页开始
    task->stack = (u32 *)stack;
    task->priority = priority;
    task->ticks = task->priority;
    task->jiffies = 0;
    task->state = TASK_READY;
    task->uid = uid;
    task->vamp = &kernel_map;
    task->pde = KERNEL_PAGE_DIR;
    task->brk = KERNEL_MEMORY_SIZE;
    task->magic = HALOS_MAGIC;//MAGIC放最后，如果栈溢出，magic就会改变

    return task;
}

//target为进入到用户态后调用的函数
//该函数由大量的临时变量，所以需要足够的空间错开
void task_to_user_mode(target_t target)
{
    task_t *task = running_task();

    //进入用户态就不能用内核空间了，得申请一块虚拟内存位图，再申请一块内核空间保存位图
    task->vamp = kmalloc(sizeof(bitmap_t));
    void *buf = (void *)alloc_kpage(1);//只申请了一页，所以按位算只能表示128M的内存空间
    bitmap_init(task->vamp, buf, PAGE_SIZE, KERNEL_MEMORY_SIZE / PAGE_SIZE);

    //创建用户进程页表，就是把内核打拷贝一份，放到用户内存里面
    task->pde = (u32)copy_pde();
    set_cr3(task->pde);

    u32 addr = (u32)task + PAGE_SIZE;
    addr -= sizeof(intr_frame_t);
    intr_frame_t *iframe = (intr_frame_t *)(addr);//从页底腾位置放中断帧

    iframe->vector = 0x20;
    iframe->edi = 1;
    iframe->esi = 2;
    iframe->ebp = 3;
    iframe->esp_dummy = 4;
    iframe->ebx = 5;
    iframe->edx = 6;
    iframe->ecx = 7;
    iframe->eax = 8;

    iframe->gs = 0;
    iframe->ds = USER_DATA_SELECTOR;
    iframe->es = USER_DATA_SELECTOR;
    iframe->fs = USER_DATA_SELECTOR;
    iframe->ss = USER_DATA_SELECTOR;
    iframe->cs = USER_CODE_SELECTOR;

    iframe->error = HALOS_MAGIC;
    
    iframe->eip = (u32)target;
    iframe->eflags = (0 << 12 | 0b10 | 1 << 9);
    iframe->esp = USER_STACK_TOP;

    //将iframe赋值到esp，跳转
    asm volatile(
        "movl %0, %%esp\n"
        "jmp interrupt_exit\n" ::"m"(iframe));
}

//设置一下刚开始的任务
static void task_setup(){
    task_t *task = running_task();
    task->magic = HALOS_MAGIC;
    task->ticks = 1;
    memset(task_table, 0, sizeof(task_table));
}

extern void interrupt_exit();
static void task_build_stack(task_t *task){
    u32 addr = (u32)task + PAGE_SIZE;
    addr -= sizeof(intr_frame_t);
    intr_frame_t *iframe = (intr_frame_t *)addr;
    iframe->eax = 0;//子进程的返回值应该为0

    addr -= sizeof(task_frame_t);
    task_frame_t *frame = (task_frame_t *)addr;
    frame->ebp = 0xaa55aa55;
    frame->ebx = 0xaa55aa55;
    frame->edi = 0xaa55aa55;
    frame->esi = 0xaa55aa55;

    frame->eip = interrupt_exit;
    task->stack = (u32 *)frame;
}

pid_t task_fork(){
    task_t *task = running_task();
    //当前进程未被阻塞，且正在执行
    assert(task->node.next == NULL && task->node.prev == NULL && task->state == TASK_RUNNING);
    task_t *child = get_free_task();
    pid_t pid = child->pid;

    //拷贝内核栈和PCB
    memcpy(child, task, PAGE_SIZE);
    child->pid = pid;
    child->ppid = task->pid;
    child->ticks = child->priority;
    child->state = TASK_READY;

    //拷贝用户进程的虚拟位图
    child->vamp = kmalloc(sizeof(bitmap_t));
    memcpy(child->vamp, task->vamp, sizeof(bitmap_t));
    
    //拷贝虚拟位图缓存
    void *buf = (void *)alloc_kpage(1);
    memcpy(buf, task->vamp->bits, PAGE_SIZE);
    child->vamp->bits = buf;

    //拷贝页目录
    child->pde = (u32)copy_pde();

    //构造child内核栈
    task_build_stack(child);

    return child->pid;
}

//差不多和fork反着来，能exit的进程一定有父进程
void task_exit(int status){
    task_t *task = running_task();
    //当前进程未被阻塞，且正在执行
    assert(task->node.next == NULL && task->node.prev == NULL && task->state == TASK_RUNNING);

    task->state = TASK_DEAD;//僵死，等待父进程收尸
    task->status = status;
    
    //释放顺序反着来
    free_pde();
    free_kpage((u32)task->vamp->bits,1);
    kfree(task->vamp);

    //把子进程托养给父进程
    for (size_t i = 0; i < NR_TASKS; i++)
    {
        task_t *child = task_table[i];
        if (!child){
            continue;
        }
        if (child->ppid != task->pid){
            continue;
        }
        child->ppid = task->ppid;
    }
    LOGK("task 0x%p exit...\n", task);

    //退出的时候如果父进程在等待，则激活父进程
    task_t *parent = task_table[task->ppid];
    if(parent->state == TASK_WAITTING && (parent->waitpid == -1 || parent->waitpid == task->pid)){
        task_unblock(parent);
    }

    schedule();
}

pid_t task_waitpid(pid_t pid, int32* status){
    task_t *task = running_task();
    task_t *child = NULL;
    
    //找到要等待的那个进程，若pid为-1，则wait所有子进程
    while (true){
        bool has_child = false;
        for (size_t i = 2; i < NR_TASKS; i++){
            task_t *ptr = task_table[i];
            if (!ptr){
                continue;
            }
            if (ptr->ppid != task->pid){
                continue;
            }
            if (pid != ptr->pid && pid != -1){
                continue;
            }
            
            //子进程退出了，收尸
            if(ptr->state == TASK_DEAD){
                child = ptr;
                task_table[i] = NULL;
                *status = child->status;
                u32 ret = child->pid;
                free_kpage((u32)child, 1);
                return ret;
            }
            //有子进程，但还没退出
            has_child = true;
        }
        //等待子进程挂掉
        if (has_child){
            task->waitpid = pid;
            task_block(task, NULL, TASK_WAITTING);
            continue;
        }
        //符合条件的子进程都没有，直接退出了
        break;
    }
    return -1;
}


extern void idle_thread();
extern void init_thread();
extern void test_thread();

void task_init(){
    list_init(&block_list);
    list_init(&sleep_list);

    task_setup();
    
    idle_task = task_create(idle_thread, "idle", 1, KERNEL_USER);
    task_create(init_thread, "init", 5, NORMAL_USER);
    task_create(test_thread, "test", 5, KERNEL_USER);
}