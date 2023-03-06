#include <halos/fifo.h>
#include <halos/assert.h>
#include <halos/debug.h>

static inline u32 fifo_next(fifo_t *fifo, u32 pos){
    return (pos + 1) % fifo->length;
}

//初始化循环队列
void fifo_init(fifo_t *fifo, char *buf, u32 length){
    fifo->buf = buf;
    fifo->length = length;
    fifo->head = 0;
    fifo->tail = 0;
}

//循环队列是否满了
bool fifo_full(fifo_t *fifo){
    bool full = (fifo_next(fifo, fifo->head) == fifo->tail);
    return full;
}

//循环队列是否为空
bool fifo_empty(fifo_t *fifo){
    return (fifo->head == fifo->tail);
}

//从循环队列中获取一个
char fifo_get(fifo_t *fifo){
    assert(!fifo_empty(fifo));
    char byte = fifo->buf[fifo->tail];
    fifo->tail = fifo_next(fifo, fifo->tail);
    return byte;
}

//往循环队列中插入一个
void fifo_put(fifo_t *fifo, char byte){
    while (fifo_full(fifo)) 
    {
        fifo_get(fifo);
    }
    fifo->buf[fifo->head] = byte;
    fifo->head = fifo_next(fifo, fifo->head);
}