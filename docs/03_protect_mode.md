# 保护模式
实模式(8086)只有1M内存，病毒可以控制全计算机  
在保护模式下，有一些寄存器只能被操作系统访问  
使用描述符来保护内存，归到部分内存只能被操作系统访问

## 全局描述符  
- 内存的起始位置
- 内存的长度/界限（长度-1）
- 内存属性

零碎的结构是为了兼容
```cpp
typedef struct descriptor //共8个字节
{
    unsigned short limit_low;//段界限0~15位
    unsigned int base_low : 24;//基地址0~23位
    unsigned char type : 4;//段类型   
    unsigned char segment : 1;//1表示代码段or数据段，0表示系统段
    unsigned char DPL : 2;//Descriptor Privilege Level描述特权等级0~3，0最高，操作系统的等级，3最低，应用程序的等级
    unsigned char present : 1;//1表示在内存中，0表示在磁盘中
    unsigned char limit_high : 4;//段界限16~19
    unsigned char available : 1;//没啥用了
    unsigned char long_mode : 1;//64位扩展标志
    unsigned char big : 1;//32位还是16位
    unsigned char granularity : 1;//粒度4KB还是1KB
    unsigned char base_high : 24;//基地址24~31位
}
```

当segment为1时，type的4位可以划分为  
|X|C/E|R/W|A|  
- A：Accessed 是否被CPU访问过
- X：1->代码段
    - C：是否依从代码段(执行的时候是否需要修改特权级)
    - R：是否可读
- X：2->数据段
    - E：0->向上扩展，1->向下扩展
    - W：是否可写

## 全局描述符表GDT
```cpp
descriptor gdt[8192]\\第0个必须全0，所以有8191个描述符
```
### 寄存器gdtr->全局描述符表的起始位置和长度
```s
lgdt [gdt_ptr];加载gdt
sgdt [gdt_ptr];保存gdt
```
```cpp
typedef struct pointer
{
    unsigned short limit;
    unsigned int base;
}_packer pointer;
```

## 段选择子
```cpp
typedef struct selector
{
    unsigned char PRL : 2;//request PL，先置为0
    unsigned char TI : 1;//0->全局描述符，1->局部描述符
    unsigned short index : 13;//全局描述符表的索引
}
```
保护模式只需要一个代码段，一个或多个数据段/栈段  
通过段选择子加载描述符到段寄存器中，会在此时校验段特权级  

## A20线
进入保护模式寻址方式改变，需要通过0x92端口打开A20线  
还需要将cr0寄存器的0位置为1，启动保护模式