#ifndef HALOS_GLOBAL_H
#define HALOS_GLOBAL_H

#include <halos/types.h>

#define GDT_SIZE 128

typedef struct descriptor_t //共8个字节
{
    unsigned short limit_low;       //段界限0~15位
    unsigned int base_low : 24;     //基地址0~23位
    unsigned char type : 4;         //段类型   
    unsigned char segment : 1;      //1表示代码段or数据段，0表示系统段
    unsigned char DPL : 2;          //Descriptor Privilege Level描述特权等级0~3，0最高，操作系统的等级，3最低，应用程序的等级
    unsigned char present : 1;      //1表示在内存中，0表示在磁盘中
    unsigned char limit_high : 4;   //段界限16~19
    unsigned char available : 1;    //没啥用了
    unsigned char long_mode : 1;    //64位扩展标志
    unsigned char big : 1;          //32位还是16位
    unsigned char granularity : 1;  //粒度4KB还是1KB
    unsigned char base_high;        //基地址24~31位
}_packed descriptor_t;

typedef struct selector_t
{
    unsigned char PRL : 2;
    unsigned char TI : 1;//0->全局描述符，1->局部描述符
    unsigned short index : 13;//全局描述符表的索引
} selector_t;

typedef struct pointer_t
{
    unsigned short limit;
    unsigned int base;
} _packed pointer_t;

void gdt_init();

#endif