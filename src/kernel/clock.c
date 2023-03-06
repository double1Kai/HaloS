#include <halos/clock.h>
#include <halos/io.h>
#include <halos/interrupt.h>
#include <halos/assert.h>
#include <halos/debug.h>
#include <halos/task.h>

#define PIT_CHAN0_REG 0X40 //计数器0端口号
#define PIT_CHAN2_REG 0X42 //计数器2端口号
#define PIT_CTRL_REG 0X43 //控制字端口号

#define HZ 100 //中断频率
#define OSCILLATOR 1193182 //振荡器频率
#define CLOCK_COUNTER (OSCILLATOR / HZ) //经过这么多计数就发生中断
#define JIFFY (1000 / HZ)

#define SPEAKER_REG 0x61
#define BEEP_HZ 440
#define BEEP_COUNTER (OSCILLATOR / BEEP_HZ)

//时间片计数器
u32 volatile jiffies = 0;//volatile关键字，不再优化，防止内存重排
u32 jiffy = JIFFY;

u32 volatile beeping = 0;

void start_beep(){
    if(!beeping){
        outb(SPEAKER_REG, inb(SPEAKER_REG) | 0b11);//开始发声
    }
    beeping = jiffies + 5;//5个时钟周期后stop
}

void stop_beep(){
    if(beeping && jiffies > beeping){
        outb(SPEAKER_REG, inb(SPEAKER_REG) & 0xfc);//停止发声
        beeping = 0;//5个时钟周期后stop
    }
}

extern void task_wakeup();

//时钟中断处理
void clock_handler(int vector){
    assert(vector == 0x20);
    send_eoi(vector);//发送中断处理结束
    stop_beep();//每次时钟中断检测是否需要stop
    task_wakeup();//唤醒结束睡眠的任务

    jiffies++;
    // DEBUGK("clock jiffies %d ...\n", jiffies);
    //获取当前执行的任务，并更新状态
    task_t *task = running_task();
    assert(task->magic == HALOS_MAGIC);
    task->jiffies = jiffies;
    task->ticks--;
    //剩余时间片耗尽
    if(!task->ticks){
        schedule();//调度
    }
}

extern u32 startup_time;
time_t sys_time(){
    //启动时间加上启动秒数
    return startup_time + (jiffies * JIFFY) / 1000;
}

//初始化计数器
void pit_init(){
    //配置计数器0时钟
    outb(PIT_CTRL_REG, 0b00110100);//0号计数器，先读取低字节，再读取高字节，模式2，二进制计数器
    outb(PIT_CHAN0_REG, CLOCK_COUNTER & 0xff);
    outb(PIT_CHAN0_REG, (CLOCK_COUNTER >> 8) &0xff);

    //配置计数器2蜂鸣器
    outb(PIT_CTRL_REG, 0b10110110);//2号计数器，先读取低字节，再读取高字节，模式3，二进制计数器
    outb(PIT_CHAN2_REG, BEEP_COUNTER & 0xff);
    outb(PIT_CHAN2_REG, (BEEP_COUNTER >> 8) &0xff);
}

void clock_init(){
    pit_init();
    set_interrupt_handler(IRQ_CLOCK, clock_handler);
    set_interrupt_mask(IRQ_CLOCK,true);
}