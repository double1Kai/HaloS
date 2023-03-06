#ifndef HALOS_FIFO_H
#define HALOS_FIFO_H

#include <halos/types.h>

//循环队列，first in frist out
typedef struct fifo_t
{
    char *buf;
    u32 length;
    u32 head;
    u32 tail;
} fifo_t;

//初始化循环队列
void fifo_init(fifo_t *fifo, char *buf, u32 length);
//循环队列是否满了
bool fifo_full(fifo_t *fifo);
//循环队列是否为空
bool fifo_empty(fifo_t *fifo);
//从循环队列中获取一个
char fifo_get(fifo_t *fifo);
//往循环队列中插入一个
void fifo_put(fifo_t *fifo, char byte);


#endif