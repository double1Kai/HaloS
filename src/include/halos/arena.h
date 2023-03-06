#ifndef HALOS_AREAN_H
#define HALOS_AREAN_H

#include <halos/types.h>
#include <halos/list.h>

//描述符数量
#define DESC_COUNT 7

//一页内存分成很多个内存块
typedef list_node_t block_t;

//内存描述符
typedef struct arena_descriptor_t{
    u32 total_block;//总块数
    u32 block_size;//每一块的大小
    list_t free_list;//空闲块的链表
} arena_descriptor_t;

//一页或多页内存
typedef struct arena_t
{
    arena_descriptor_t *desc;//该arena的描述符
    u32 count;//当前剩余多少块
    u32 large;//是否超过了1024，超过了的话count表示页数
    u32 magic;//魔数
} arena_t;

void *kmalloc(size_t size);
void kfree(void *ptr);


#endif