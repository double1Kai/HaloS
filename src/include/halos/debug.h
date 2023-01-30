#ifndef HALOS_DEBUG_H
#define HALOS_DEBUG_H

//相当于一个会打印文件和行数的printk
void debugk(char *file, int line, const char *fmt, ...);

#define BMB asm volatile("xchgw %bx, %bx")//bochs魔术断点
#define DEBUGK(fmt, args...) debugk(__BASE_FILE__, __LINE__, fmt, ##args)

// #define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#endif