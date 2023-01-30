#ifndef HALOS_MEMORY_H
#define HALOS_MEMORY_H

#include <halos/types.h>

#define PAGE_SIZE 0x1000     // 一页的大小 4K
#define MEMORY_BASE 0x100000 // 1M，可用内存开始的位置

void memory_map_init();

typedef struct page_entry_t{
    u8 present : 1; //1在内存中，0不在
    u8 write : 1; //0只读，1可读可写
    u8 user : 1; //1所有人，0超级用户，DPL<3
    u8 pwt : 1;//page write through 1直写模式，0写回模式
    u8 pcd : 1;//page cache disabled 1禁止该页缓冲
    u8 access : 1;//1被访问过，换入换出的时候用得上
    u8 dirty : 1;//页缓冲是否被写过
    u8 pat : 1;//page attribute table 页大小 0 4k，1 4M
    u8 global : 1;//1全局，所有进程都用到了，不刷新缓冲，进程切换的时候不用换这一页
    u8 ignore : 3;//闲置
    u32 index : 20;//页索引
}_packed page_entry_t;

u32 get_cr3();
void set_cr3(u32 pde);

#endif