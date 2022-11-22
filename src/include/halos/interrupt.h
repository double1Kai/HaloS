#ifndef HALOS_INTERRUPT_H
#define HALOS_INTERRUPT_H

#include <halos/types.h>

#define IDE_SIZE 256

typedef struct gate_t{//共8个字节
    u16 offset0; //段内偏移0~15位
    u16 selector; //代码段选择子
    u8 reserved; //保留不用，全为0
    u8 type : 4;//描述符类型 任务门/中断门/陷阱门
    u8 segment: 1; //segment = 0表示系统段
    u8 DPL : 2; //使用int指令访问的最低权限
    u8 present : 1; //是否有效(是否在内存中)
    u16 offset1; //段内偏移16~31位
}_packed gate_t;

typedef void *handler_t; // 中断处理函数入口

void interrupt_init();

#endif