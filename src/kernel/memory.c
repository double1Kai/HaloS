#include <halos/memory.h>
#include <halos/types.h>
#include <halos/debug.h>
#include <halos/assert.h>
#include <halos/stdlib.h>
#include <halos/string.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define ZONE_VALID 1    //ards可用区域
#define ZONE_RESERVED 2 //ards不可用区域

#define IDX(addr) ((u32)addr >> 12) //获取addr的页索引(第几页)
#define DIDX(addr) (((u32)addr >> 22) & 0x3ff)  //获取addr的页目录索引（高10位）
#define TIDX(addr) (((u32)addr >> 12) & 0x3ff)  //获取addr的页表索引 （中10位）
#define PAGE(idx) ((u32)idx << 12) //获取页索引idx对应的页的起始位置
#define ASSERT_PAGE(addr) assert((addr & 0xfff) == 0) //addr的后三位必须为0，即addr必须是页开始的位置，否则出错

//内核页目录索引
#define KERNEL_PAGE_DIR 0x1000

//内核页表索引
static u32 KERNEL_PAGE_TABLE[] = {
    0x2000,
    0x3000
};

//一个页表可以映射4M，这里一共8M
#define KERNEL_MEMORY_SIZE (0x100000 * sizeof(KERNEL_PAGE_TABLE))

typedef struct ards_t
{
    u64 base; //内存基地址
    u64 size; //内存长度
    u32 type; //内存类型
}_packed ards_t;

static u32 memory_base = 0; // 可用内存基地址，应该等于 1M
static u32 memory_size = 0; // 可用内存大小
static u32 total_pages = 0; // 所有内存页数
static u32 free_pages = 0;  // 空闲内存页数

#define used_pages (total_pages - free_pages)

void memory_init(u32 magic, u32 addr){
    u32 count;
    ards_t* ptr;

    if(magic == HALOS_MAGIC){
        count = *(u32 *)addr;
        ptr = (ards_t *)(addr + 4);//loader.asm中，addr往后4个字节就是ards_buf
        for (size_t i = 0; i < count; i++, ptr++){
            LOGK("Memory base 0x%p size 0x%p type 0x%p\n",(u32)ptr->base,(u32)ptr->size,(u32)ptr->type);
            if(ptr->type == ZONE_VALID && ptr->size > memory_size){
                memory_base = (u32)ptr->base;
                memory_size = (u32)ptr->size;//保持最大内存
            }
        }
    }
    else{
        panic("MEMORY INIT MAGIC UNKNOW 0x%p\n", magic);
    }
    LOGK("ARDS count %d\n",count);
    LOGK("Memory base 0x%p\n",(u32)memory_base);
    LOGK("Memory size 0x%p\n",(u32)memory_size);

    assert(memory_base == MEMORY_BASE);//最大内存的开始位置应该为1M
    assert((memory_size & 0xff) == 0); //应该按页对齐，内存应该能被页长整除

    total_pages = IDX(memory_size) + IDX(MEMORY_BASE);
    free_pages = IDX(memory_size);

    LOGK("Total pages %d\n", total_pages);
    LOGK("Free pages %d\n", free_pages);

    if(memory_size < KERNEL_MEMORY_SIZE){
        panic("System Memory Is %dM too small, at least %dM needed\n",
        memory_size/MEMORY_BASE, KERNEL_MEMORY_SIZE/MEMORY_BASE);
    }

}

static u32 start_page = 0;  //可分配物理内存起始地址
static u8 *memory_map;      //物理内存数组
static u32 memory_map_pages; //物理内存数组占用的页数

void memory_map_init(){
    //初始化物理内存数组
    memory_map = (u8 *)memory_base;
    //计算物理内存数组占用的页数
    memory_map_pages = div_round_up(total_pages, PAGE_SIZE);//一页可以管理4K页
    LOGK("Memory map page count %d\n", memory_map_pages);

    free_pages -= memory_map_pages;

    //物理内存数组清零
    memset((void *)memory_map, 0, memory_map_pages * PAGE_SIZE);

    //前1M内存和物理内存使用的页都被占用
    start_page = IDX(MEMORY_BASE) + memory_map_pages;
    for(size_t i = 0; i < start_page; i++){
        memory_map[i] = 1;//引用数量置为1
    }

    LOGK("Total Pages %d Free Pages %d\n",total_pages, free_pages);
}

//分配一页物理内存
static u32 get_page(){
    for(size_t i = start_page; i < total_pages; i++){
        //若未被占用
        if(!memory_map[i]){
            memory_map[i] = 1;
            free_pages--;
            assert(free_pages >= 0);
            u32 page = ((u32)i) << 12;
            LOGK("Get Page 0x%p\n", page);
            return page;
        }
    }
    panic("Out of Memory!!!");
}
//释放一页物理内存
static void put_page(u32 addr){
    ASSERT_PAGE(addr);
    u32 idx = IDX(addr);
    //idx必须是能分配的页数
    assert(idx >= start_page && idx < total_pages);
    //idx必须被引用
    assert(memory_map[idx] >= 1);
    memory_map[idx] -= 1;
    //若减1后为0，则增加一页空闲页
    if(!memory_map[idx]){
        free_pages++;
    }
    assert(free_pages > 0 && free_pages < total_pages);
    LOGK("Put Page 0x%p\n", addr);
}

u32 get_cr3(){
    asm volatile("movl %cr3, %eax\n");
}

//pbe为页目录起始地址
void set_cr3(u32 pde){
    ASSERT_PAGE(pde);
    asm volatile("movl %%eax, %%cr3\n" ::"a"(pde));//a，在执行汇编指令前，将pbe放到eax中
}

//cr0最高位置为1，启用分页
static _inline void enable_page(){
    asm volatile(
        "movl %cr0, %eax\n"
        "orl $0x80000000, %eax\n"
        "movl %eax, %cr0");
}

//初始化页表项
static void entry_init(page_entry_t *entry, u32 index){
    *(u32 *)entry = 0;
    entry->present = 1;//存在
    entry->write = 1;//可写
    entry->user = 1;//所有人都能访问
    entry->index = index;
}

//初始化内存映射
void mapping_init(){
    page_entry_t *pde = (page_entry_t *)KERNEL_PAGE_DIR;
    memset(pde, 0, PAGE_SIZE);

    idx_t index = 0;

    for (idx_t didx = 0; didx < (sizeof(KERNEL_PAGE_TABLE) / 4); didx++)
    {
        page_entry_t *pte = (page_entry_t *)KERNEL_PAGE_TABLE[didx];
        memset(pte, 0, PAGE_SIZE);

        page_entry_t *dentry  = &pde[didx];
        entry_init(dentry, IDX((u32)pte));//页目录的前两项改为2007和3007
        
        for(size_t tidx = 0; tidx < 1024; tidx++,index++){
            if (index == 0){
                continue;//第0页不映射，防止出错，因为空指针一般都是0
            }
            page_entry_t *tentry  = &pte[tidx];
            entry_init(tentry, index);
            memory_map[index] = 1;//该页被占用
        }
    }
    
    //最后一个页表指向自己，便于修改
    page_entry_t *entry = &pde[1023];
    entry_init(entry, IDX(KERNEL_PAGE_DIR));

    set_cr3((u32)pde);
    BMB;
    enable_page();
}
static page_entry_t *get_pde(){
    //通过页目录最后一项找到页目录（此时视作页表），再通过页表的最后一项找到页目录（此时视作普通页）
    return(page_entry_t *)(0xfffff000);
}
static page_entry_t *get_pte(u32 vaddr){
    //通过页目录最后一项找到页目录（此时视作页表），再通过页表找到页表（此时视作普通页）
    return(page_entry_t *)(0xffc00000 | (DIDX(vaddr)<<12));
}
//刷新块表TLB
static void flush_tlb(u32 vaddr){
    asm volatile("invlpg (%0)" ::"r"(vaddr)
                 : "memory");
}
