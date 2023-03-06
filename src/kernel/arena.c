#include <halos/arena.h>
#include <halos/memory.h>
#include <halos/string.h>
#include <halos/stdlib.h>
#include <halos/assert.h>

extern u32 free_pgaes();
static arena_descriptor_t descriptors[DESC_COUNT];

//arena初始化
void arena_init(){
    u32 block_size = 16;//最小一块16B
    for (size_t i = 0; i < DESC_COUNT; i++)
    {
        arena_descriptor_t *desc = &descriptors[i];
        desc->block_size = block_size;
        desc->total_block = (PAGE_SIZE - sizeof(arena_t)) / block_size;
        list_init(&desc->free_list);
        block_size <<= 1; //blocksize * 2，最大的是1024B
    }
}

//获得arena第idx块内存指针
static void *get_arena_block(arena_t *arena, u32 idx){
    assert(arena->desc->total_block > idx);
    void *addr = (void *)(arena + 1);//跳过页开始存储arena的地址，到可以分配的第0个块的地址
    u32 gap = idx * arena->desc->block_size;//跳过idx个块的地址
    return addr + gap;
}

//用块地址获取对应的arena地址
static arena_t *get_block_arena(block_t *block){
    return (arena_t *)((u32)block & 0xfffff000);
}

void *kmalloc(size_t size){
    arena_descriptor_t *desc = NULL;
    arena_t *arena;
    block_t *block;
    char *addr;
    //大于1024就不分配块了，直接分配页吧，页首还得存arena信息
    if(size > 1024){
        u32 asize = size + sizeof(arena_t);
        u32 count = div_round_up(asize, PAGE_SIZE);

        arena = (arena_t *)alloc_kpage(count);
        memset(arena, 0, count * PAGE_SIZE);
        arena->large = true;//表示size大于1024，意味着count是页数
        arena->count = count;
        arena->desc = NULL;
        arena->magic = HALOS_MAGIC;

        addr = (char *)((u32)arena + sizeof(arena_t));
        return addr;
    }
    //小于1024就在描述符表里找
    for (size_t i = 0; i < DESC_COUNT; i++)
    {
        desc = &descriptors[i];
        if(desc->block_size > size){
            break;
        }
    }
    assert(desc != NULL);
    //如果freelist中没有了对应大小的块，就申请一页内存
    if(list_empty(&desc->free_list)){
        arena = (arena_t *)alloc_kpage(1);
        memset(arena, 0, PAGE_SIZE);
        arena->desc = desc;
        arena->large = false;
        arena->count = desc->total_block;
        arena->magic = HALOS_MAGIC;

        //遍历整个arena，把所有块都放入空闲队列
        for (size_t i = 0; i < desc->total_block; i++)
        {   
            block = get_arena_block(arena, i);
            assert(!list_search(&arena->desc->free_list, block));
            list_push(&arena->desc->free_list, block);
            assert(list_search(&arena->desc->free_list, block));
        }
    }
    
    //这下肯定有了，分配一个
    block = list_pop(&desc->free_list);
    arena = get_block_arena(block);
    assert(arena->magic == HALOS_MAGIC && !arena->large);

    // memset(block, 0, desc->block_size);

    arena->count--;
    return block;
}

void kfree(void *ptr){
    assert(ptr);
    block_t *block = (block_t *)ptr;
    arena_t *arena = get_block_arena(block);

    assert(arena->large == 1 || arena->large == 0);
    assert(arena->magic == HALOS_MAGIC);
    
    //直接free页
    if(arena->large){
        free_kpage((u32)arena, arena->count);
        return;
    }

    list_push(&arena->desc->free_list, block);
    arena->count++;
    //如果这一页所有的块都被释放了，就把这一页都释放了
    if (arena->count == arena->desc->total_block)
    {
        for (size_t i = 0; i < arena->desc->total_block; i++)
        {
            block = get_arena_block(arena, i);
            assert(list_search(&arena->desc->free_list, block));
            list_remove(block);
            assert(!list_search(&arena->desc->free_list, block));
        }
        free_kpage((u32)arena, 1);
    }
}