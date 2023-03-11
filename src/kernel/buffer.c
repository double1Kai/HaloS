#include <halos/buffer.h>
#include <halos/memory.h>
#include <halos/debug.h>
#include <halos/assert.h>
#include <halos/device.h>
#include <halos/string.h>
#include <halos/task.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define HASH_COUNT 31 //哈希表长度取素数

static buffer_t *buffer_start = (buffer_t *)KERNEL_BUFFER_MEM;//buffer起始位置
static u32 buffer_count = 0;//当前buffer数量

//当前buffer_t的结构体位置，初始与起始位置相同
static buffer_t *buffer_ptr = (buffer_t *)KERNEL_BUFFER_MEM;

//当前数据区的位置，数据区和buffer是对称着来的，buffer++，data就要减去一个blocksize
static void *buffer_data = (void *)(KERNEL_BUFFER_MEM + KERNEL_BUFFER_SIZE - BLOCK_SIZE);

static list_t free_list;    //缓存链表，被释放的块
static list_t wait_list;    //等待进程链表
static list_t hash_table[HASH_COUNT];

//哈希函数，搬来的
u32 hash(dev_t dev, idx_t block){
    return (dev ^ block) % HASH_COUNT;
}

//从哈希表中得到设备对应的buffer，不存在则返回空
static buffer_t *get_from_hash_table(dev_t dev, idx_t block){
    u32 idx = hash(dev, block);
    list_t *list = &hash_table[idx];
    buffer_t *bf = NULL;

    for(list_node_t *node = list->head.next; node != &list->tail; node = node->next){
        buffer_t *ptr= element_entry(buffer_t, hnode, node);
        if (ptr->dev == dev && ptr->block == block)
        {
            bf = ptr;
            break;
        }
    }
    if(!bf){
        return NULL;
    }
    
    //如果找得到且bf在空闲列表中，将其移除
    if(list_search(&free_list, &bf->rnode)){
        list_remove(&bf->hnode);
    }
    return bf;
}

//将bf放入哈希表
static void hash_locate(buffer_t *bf){
    u32 idx = hash(bf->dev, bf->block);
    list_t *list = &hash_table[idx];
    assert(!list_search(list, &bf->hnode));
    list_push(list, &bf->hnode);
}

//将bf从哈希表中移除
static void hash_remove(buffer_t *bf){
    u32 idx = hash(bf->dev, bf->block);
    list_t *list = &hash_table[idx];
    assert(!list_search(list, &bf->hnode));
    list_remove(&bf->hnode);
}

//没有一次性初始化全部，用到才初始化
static buffer_t *get_new_buffer(){
    buffer_t *bf = NULL;

    //新增一个buffer不会侵占到data的内存
    if((u32)buffer_ptr + sizeof(buffer_t) < (u32)buffer_data){
        bf = buffer_ptr;
        bf->data = buffer_data;
        bf->dev = EOF;
        bf->block = 0;
        bf->count = 0;
        bf->dirty = false;
        bf->valid = false;
        lock_init(&bf->lock);
        buffer_count++;
        buffer_ptr++;
        buffer_data -= BLOCK_SIZE;
        LOGK("buffer count %d\n", buffer_count);
    }
    return bf;
}

//获取一个空闲的buffer
static buffer_t *get_free_buffer(){
    buffer_t *bf = NULL;
    while (true)
    {   
        //如果还有空
        bf = get_new_buffer();
        if(bf){
            return bf;
        }

        //否则从空闲链表里获取
        if(!list_empty(&free_list)){
            //取最远的未访问过的块，因为是往头结点后面插入，越久的越靠后
            bf = element_entry(buffer_t, rnode, list_popback(&free_list));
            hash_remove(bf);
            bf->valid = false;
            return bf;
        }

        task_block(running_task(), &wait_list, TASK_BLOCKED);
    }
}

//获取dev block对应的缓冲
buffer_t *getblk(dev_t dev, idx_t block){
    buffer_t *bf = get_from_hash_table(dev, block);
    if (bf)
    {
        assert(bf->valid);
        return bf;
    }
    bf = get_free_buffer();
    assert(bf->count==0);
    assert(bf->dirty==0);

    bf->count = 1;
    bf->dev = dev;
    bf->block = block;
    hash_locate(bf);
    return bf;
}

//读取dev设备的block块
buffer_t *bread(dev_t dev, idx_t block){
    buffer_t *bf = getblk(dev, block);
    assert(bf != NULL);
    if (bf->valid)
    {
        bf->count++;
        return bf;
    }
    
    device_request(bf->dev, bf->data, BLOCK_SECS, bf->block * BLOCK_SECS, 0, REQ_READ);
    bf->dirty = false;
    bf->valid = true;
    
    return bf;
}

//写dev设备的block块
void bwrite(buffer_t *bf){
    assert(bf);
    if(!bf->dirty){
        return;
    }
    device_request(bf->dev, bf->data, BLOCK_SECS, bf->block * BLOCK_SECS, 0, REQ_WRITE);

    bf->dirty = false;
    bf->valid = true;
}

//释放缓冲
void brelse(buffer_t *bf){
    if (!bf)
    {
        return;
    }
    if(bf->dirty){
        bwrite(bf);
    }
    bf->count--;
    assert(bf->count >= 0);
    //如果还有人用，直接返回
    if(bf->count){
        return;
    }

    assert(!bf->rnode.next);
    assert(!bf->rnode.next);
    list_push(&free_list, &bf->rnode);

    //唤醒等待链表中的进程
    if(!list_empty(&wait_list)){
        task_t *task = element_entry(task_t, node, list_popback(&wait_list));
        task_unblock(task);
    }
}

void buffer_init(){
    LOGK("buffer_t size is %d\n", sizeof(buffer_t));

    //初始化空闲链表
    list_init(&free_list);
    //初始化等待进程链表
    list_init(&wait_list);

    //初始化哈希表
    for (size_t i = 0; i < HASH_COUNT; i++)
    {
        list_init(&hash_table[i]);
    }
    
}