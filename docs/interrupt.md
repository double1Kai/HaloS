## 中断函数

- 内中断
    - 软中断
        -系统调用
    - 异常
        - 除0异常
        - 指令错误
        - 缺页错误
- 外中断
    - 时钟中断
    - 键盘中断
    - 硬盘中断

调用中断函数用int 和 iret  
int 会保存eip cs（中断函数可能在任何地方，所以要保存） eflags， call只会保存eip

## 中断向量表 实模式
> `0x000` ~ `0x3ff`
4个字节表示一个中断向量，总计256个中断向量  
前两个字节是偏移地址，后两个字节是段地址
int 中断向量编号

## 中断描述符 保护模式
可以放在内存中的任何位置
中断向量 -> 中断描述符
中断向量表 -> 中断描述符表

中断描述符：
- 代码段(cs)
- 段内偏移地址(eip)
- 一些属性

```c++
typedef struct gate_t{//共8个字节
    u16 offset0; //段内偏移0~15位
    u16 selector; //代码段选择子
    u8 reserved; //保留不用，全为0
    u8 type : 4;//描述符类型 任务门/中断门/陷阱门
    u8 segment: 1; //segment = 0表示系统段
    u8 DPL : 2; //使用int指令访问的最低权限
    u8 present : 1; //是否有效(是否在内存中)
    u16 offset1;//段内偏移16~31位
}_packed gate_t;
```
type segment = 0:
    - 0b0101 -> 任务门，复杂低效，64位时就去掉了
    - 0b1110 -> 中断门，IF位自动置为0
    - 0b1111 -> 陷阱门，IF不会自动置为0 

中断描述符表
```c++
gate_t idt[IDT_SIZE];
```

中断描述符表寄存器idtr，中断描述符表的起始位置和长度

```s
lidtr [idt_ptr];加载 idt
sidtr [idt_ptr];保存 idt
```
```cpp
typedef struct pointer
{
    unsigned short limit;
    unsigned int base;
}_packed pointer;
```

## 异常
- 故障，可以被修复的异常
- 陷阱，通常用于调试
- 终止，最严重的异常

## 调试器
- 不能影响程序执行
- 可以在断点处停下来

## 外中断
