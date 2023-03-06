#ifndef HALOS_MEMORY_H
#define HALOS_MEMORY_H

#include <halos/types.h>

#define PAGE_SIZE 0x1000     // 一页的大小 4K
#define MEMORY_BASE 0x100000 // 1M，可用内存开始的位置

//内核占用8M内存
#define KERNEL_MEMORY_SIZE 0x800000

//用户栈顶地址128M
#define USER_STACK_TOP 0x8000000

//用户栈最多2M
#define USER_STACK_SIZE 0x200000

//用户栈底地址 128 - 2 M
#define USER_STACK_BOTTOM (USER_STACK_TOP - USER_STACK_SIZE)

void memory_map_init();

//内核页目录索引
#define KERNEL_PAGE_DIR 0x1000

//内核页表索引
static u32 KERNEL_PAGE_TABLE[] = {
    0x2000,
    0x3000
};

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

//获取cr2寄存器，存放了缺页异常时访问的地址
u32 get_cr2();

//获取cr3寄存器
u32 get_cr3();

//设置cr3寄存器，参数为页目录地址
void set_cr3(u32 pde);

//分配count个连续的内核页
u32 alloc_kpage(u32 count);

//释放连续的count个内核页
void free_kpage(u32 vaddr, u32 count);

//将vaddr映射为物理内存
void link_page(u32 vaddr);

//去掉vaddr对物理内存的映射
void unlink_page(u32 vaddr);

//拷贝页目录
page_entry_t *copy_pde();

//释放页目录
void free_pde();

//系统调用brk
int32 sys_brk(void *addr);

#endif