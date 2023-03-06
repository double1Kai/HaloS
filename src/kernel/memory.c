#include <halos/memory.h>
#include <halos/types.h>
#include <halos/debug.h>
#include <halos/assert.h>
#include <halos/stdlib.h>
#include <halos/string.h>
#include <halos/bitmap.h>
#include <halos/multiboot2.h>
#include <halos/task.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define ZONE_VALID 1    //ards可用区域
#define ZONE_RESERVED 2 //ards不可用区域

#define IDX(addr) ((u32)addr >> 12) //获取addr的页索引(第几页)
#define DIDX(addr) (((u32)addr >> 22) & 0x3ff)  //获取addr的页目录索引（高10位）
#define TIDX(addr) (((u32)addr >> 12) & 0x3ff)  //获取addr的页表索引 （中10位）
#define PAGE(idx) ((u32)idx << 12) //获取页索引idx对应的页的起始位置
#define ASSERT_PAGE(addr) assert((addr & 0xfff) == 0) //addr的后三位必须为0，即addr必须是页开始的位置，否则出错

#define PDE_MASK 0xFFC00000

//内核虚拟内存位图放这里
#define KERNEL_MAP_BITS 0x4000

bitmap_t kernel_map;

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
    u32 count = 0;

    if(magic == HALOS_MAGIC){
        count = *(u32 *)addr;

        ards_t* ptr = (ards_t *)(addr + 4);//loader.asm中，addr往后4个字节就是ards_buf
        for (size_t i = 0; i < count; i++, ptr++){
            LOGK("Memory base 0x%p size 0x%p type 0x%p\n",(u32)ptr->base,(u32)ptr->size,(u32)ptr->type);
            if(ptr->type == ZONE_VALID && ptr->size > memory_size){
                memory_base = (u32)ptr->base;
                memory_size = (u32)ptr->size;//保持最大内存
            }
        }
    }
    else if(magic == MULTIBOOT2_MAGIC){//multiboot2启动，相当于grub自动内存检测了吧，抄的，反正也不用
        u32 size = *(unsigned int *)addr;
        multi_tag_t *tag = (multi_tag_t *)(addr + 8);

        LOGK("Announced mbi size 0x%x\n", size);
        while (tag->type != MULTIBOOT_TAG_TYPE_END)
        {
            if (tag->type == MULTIBOOT_TAG_TYPE_MMAP)
                break;
            // 下一个 tag 对齐到了 8 字节
            tag = (multi_tag_t *)((u32)tag + ((tag->size + 7) & ~7));
        }

        multi_tag_mmap_t *mtag = (multi_tag_mmap_t *)tag;
        multi_mmap_entry_t *entry = mtag->entries;
        while ((u32)entry < (u32)tag + tag->size)
        {
            LOGK("Memory base 0x%p size 0x%p type %d\n",
                 (u32)entry->addr, (u32)entry->len, (u32)entry->type);
            count++;
            if (entry->type == ZONE_VALID && entry->len > memory_size)
            {
                memory_base = (u32)entry->addr;
                memory_size = (u32)entry->len;
            }
            entry = (multi_mmap_entry_t *)((u32)entry + mtag->entry_size);
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

    //初始化内核虚拟内存位图，8位对齐
    u32 length = (IDX(KERNEL_MEMORY_SIZE) - IDX(MEMORY_BASE)) / 8;
    bitmap_init(&kernel_map, (u8 *)KERNEL_MAP_BITS, length, IDX(MEMORY_BASE));
    //memory_map占用的页数先分配出去
    bitmap_scan(&kernel_map, memory_map_pages);
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

u32 get_cr2(){
    asm volatile("movl %cr2, %eax\n");
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

    enable_page();
}

//通过页目录最后一项找到页目录（此时视作页表），再通过页表的最后一项找到页目录（此时视作普通页）
static page_entry_t *get_pde(){
    return(page_entry_t *)(0xfffff000);
}

//获取虚拟地址vaddr对应的页表，如果页表不存在，则创建
static page_entry_t *get_pte(u32 vaddr, bool create){
   page_entry_t *pde = get_pde();//搞到页目录
   u32 idx = DIDX(vaddr);
   page_entry_t *entry = &pde[idx];//搞到页表入口

    //要么不创建且页表在内存中，要么创建
    assert(create || (!create && entry->present));

    page_entry_t* table = (page_entry_t *)(PDE_MASK | (idx << 12));//找了一轮变成可以修改的页表
    
    //创建一个页表，由于局部性原理，创建的页表可以不用销毁，反正全连接完也就4M
    if(!entry->present){
        LOGK("Get and Create Page Table Entry for 0x%p\n", vaddr);
        u32 page = get_page();
        entry_init(entry, IDX(page));
        memset(table, 0, PAGE_SIZE);
    }
    return table;
}

//刷新快表TLB
static void flush_tlb(u32 vaddr){
    asm volatile("invlpg (%0)" ::"r"(vaddr)
                 : "memory");
}

//从位图中扫描count个连续的页
static u32 scan_page(bitmap_t *map, u32 count){
    assert(count > 0);
    int32 index = bitmap_scan(map, count);
    if(index == EOF){
        panic("Scan Page Fail!!!");
    }
    u32 addr = PAGE(index);
    LOGK("Scan Page 0x%p count %d\n", addr, count);
    return addr;
}

//与scan_page相对应，重置相应的页
static void reset_page(bitmap_t *map, u32 addr, u32 count){
    ASSERT_PAGE(addr);
    assert(count > 0);
    u32 index = IDX(addr);
    for(size_t i = 0; i < count; i++){
        assert(bitmap_test(map, index + i));
        bitmap_set(map, index + i, 0);
    }
}

//分配count个连续的内核页
u32 alloc_kpage(u32 count){
    assert(count > 0);
    u32 vaddr = scan_page(&kernel_map, count);
    LOGK("Alloc Kernel Page 0x%p count %d\n", vaddr, count);
    return vaddr;
}

//释放连续的count个内核页
void free_kpage(u32 vaddr, u32 count){
    ASSERT_PAGE(vaddr);
    assert(count > 0);
    reset_page(&kernel_map, vaddr, count);
    LOGK("Free Kernel Page 0x%p count %d\n", vaddr, count);
}

//将vaddr映射为物理内存
void link_page(u32 vaddr){
    //vaddr必须是页开始的位置
    ASSERT_PAGE(vaddr);
    
    //获取页框对应的entry
    page_entry_t *pte = get_pte(vaddr, true);
    page_entry_t *entry = &pte[TIDX(vaddr)];

    task_t *task = running_task();
    bitmap_t *map = task->vamp;
    u32 index = IDX(vaddr);

    //如果页面已经存在于内存中，则位图一定被占用，直接返回
    if(entry->present){
        assert(bitmap_test(map, index));
        return;
    }
    //反之，一定没有
    assert(!bitmap_test(map, index));
    bitmap_set(map, index, true);

    u32 paddr = get_page();
    entry_init(entry, IDX(paddr));
    flush_tlb(vaddr);
    LOGK("Link From 0x%p to 0x%p\n", vaddr, paddr);
}

//去掉vaddr对物理内存的映射
void unlink_page(u32 vaddr){
    ASSERT_PAGE(vaddr);

    //获取页框对应的entry
    page_entry_t *pte = get_pte(vaddr, true);
    page_entry_t *entry = &pte[TIDX(vaddr)];

    task_t *task = running_task();
    bitmap_t *map = task->vamp;
    u32 index = IDX(vaddr);

    //与link对应
    if(!entry->present){
        assert(!bitmap_test(map, index));
        return;
    }

    assert(entry->present && bitmap_test(map, index));

    entry->present = false;
    bitmap_set(map, index, false);

    u32 paddr = PAGE(entry->index);

    DEBUGK("Unlink From 0x%p to 0x%p\n", vaddr, paddr);
    //释放，putpage内会判断是否真的需要释放
    put_page(paddr);
    flush_tlb(vaddr);
}

//拷贝一页，返回拷贝后的物理地址
static u32 copy_page(void *page){
    u32 paddr = get_page();
    //第0页没有映射，拿过来用一用
    page_entry_t *entry = get_pte(0, false);
    entry_init(entry, IDX(paddr));
    memcpy((void *)0, (void *)page, PAGE_SIZE);
    //用完就丢
    entry->present = false;
    return paddr;
}

//拷贝当前的页目录
page_entry_t *copy_pde(){
    task_t *task = running_task();
    page_entry_t *pde = (page_entry_t *)alloc_kpage(1);
    memcpy(pde, (void *)task->pde, PAGE_SIZE);
    
    //将最有一个页指向自己，便于修改
    page_entry_t *entry = &pde[1023];
    entry_init(entry, IDX(pde));

    //遍历页目录
    page_entry_t *dentry;
    //从2开始，即用户态的页表
    for (size_t didx = 2; didx < 1023; didx++){
        dentry = &pde[didx];
        if (!dentry->present){
            continue;
        }
        //如果页表存在，就接着遍历页表
        page_entry_t *pte = (page_entry_t *)(PDE_MASK | (didx << 12));
        for (size_t tidx = 0; tidx < 1024; tidx++)
        {
            entry = &pte[tidx];
            if(!entry->present){
                continue;
            }
            //如果页框存在，则其至少有一个引用，将其置为只读
            assert(memory_map[entry->index] > 0);
            entry->write = false;
            //同时引用+1，新进程也用了
            memory_map[entry->index]++;
            assert(memory_map[entry->index] < 255);
        }
        //子进程的页表指向新的拷贝后的页表
        u32 paddr = copy_page(pte);
        dentry->index = IDX(paddr);
    }
    //刷新块表
    set_cr3(task->pde);

    return pde;
}

//释放页目录
void free_pde(){
    task_t *task = running_task();
    assert(task->uid != KERNEL_USER);
    page_entry_t *pde = get_pde();
    //遍历页目录，如果页表存在则遍历页表，如果页框存在则释放，遍历完页表后再释放页表，最后释放页目录
    for (size_t didx = 2; didx < 1023; didx++){
        page_entry_t *dentry = &pde[didx];
        if(!dentry->present){
            continue;
        }
        page_entry_t *pte = (page_entry_t *)(PDE_MASK | (didx << 12));
        for (size_t tidx = 0; tidx < 1024; tidx++){
            page_entry_t * entry = &pte[tidx];
            if(!entry->present){
                continue;
            }

            assert(memory_map[entry->index] > 0);
            put_page(PAGE(entry->index));
        }
        put_page(PAGE(dentry->index));
    }

    free_kpage(task->pde, 1);
    LOGK("free pages %d\n", free_pages); 
}

int32 sys_brk(void *addr){
    LOGK("task brk 0x%p\n", addr);
    u32 brk = (u32)addr;
    ASSERT_PAGE(brk);//必须为页开始的位置

    task_t* task = running_task();
    //该系统调用只能有内核态发起，且地址必须有效
    assert(task->uid != KERNEL_USER);
    assert(KERNEL_MEMORY_SIZE < brk < USER_STACK_BOTTOM);

    u32 old_brk = task->brk;
    //原brk若大于新brk，则释放多的内存
    if(old_brk > brk){
        for (u32 page = brk; page < old_brk; page + PAGE_SIZE){
            unlink_page(page);
        }
    }else if(IDX(brk - old_brk) > free_pages){
        return -1;//超了
    }

    task->brk = brk;//就不去link了，缺页中断的时候自己会link
    return 0;
}

typedef struct page_error_code_t
{
    u8 present : 1;
    u8 write : 1;
    u8 user : 1;
    u8 reserved0 : 1;
    u8 fetch : 1;
    u8 protection : 1;
    u8 shadow : 1;
    u16 reserved1 : 8;
    u8 sgx : 1;
    u16 reserved2;
} _packed page_error_code_t;

void page_fault(
    u32 vector,
    u32 edi, u32 esi, u32 ebp, u32 esp,
    u32 ebx, u32 edx, u32 ecx, u32 eax,
    u32 gs, u32 fs, u32 es, u32 ds,
    u32 vector0, u32 error, u32 eip, u32 cs, u32 eflags)
{
    assert(vector == 0xe);
    u32 vaddr = get_cr2();
    LOGK("fault address 0x%p\n", vaddr);

    page_error_code_t *code = (page_error_code_t *)&error;
    task_t *task = running_task();

    //必须是用户态的地址
    assert(KERNEL_MEMORY_SIZE <= vaddr && vaddr < USER_STACK_TOP);

    //如果是写了只读的页
    if(code->present){
        assert(code->write);
        page_entry_t *pte = get_pte(vaddr, false);
        page_entry_t *entry = &pte[TIDX(vaddr)];

        assert(entry->present);
        assert(memory_map[entry->index] > 0);
        if(memory_map[entry->index] == 1){
            //引用减到了1，直接打开写就可以了
            entry->write = true;
            LOGK("Write page for 0x%p\n", vaddr);
        }else{
            //复制到新的一页，在初始化的时候会将write置为true，原本那页就是子进程的了
            void *page = (void *)PAGE(IDX(vaddr));
            u32 paddr = copy_page(page);
            memory_map[entry->index]--;
            entry_init(entry, IDX(paddr));
            flush_tlb(vaddr);
            LOGK("Copy page for 0x%p\n", vaddr);
        }
        return;
    }
    
    //如果不存在于内存且未栈溢出
    if (!code->present && (vaddr < task->brk || vaddr >= USER_STACK_BOTTOM))
    {
        u32 page = PAGE(IDX(vaddr));
        link_page(page);
        // BMB;
        return;
    }
    panic("page fault!!!");
}