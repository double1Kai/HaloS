#include <halos/fs.h>
#include <halos/debug.h>
#include <halos/bitmap.h>
#include <halos/assert.h>
#include <halos/string.h>
#include <halos/buffer.h>
#include <halos/arena.h>
#include <halos/syscall.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define INODE_NR 64

static inode_t inode_table[INODE_NR];

//申请一个inode
static inode_t *get_free_inode(){
    for (size_t i = 0; i < INODE_NR; i++)
    {
        inode_t *inode = &inode_table[i];
        if (inode->dev == EOF)
        {
            return inode;
        }
    }
    panic("no more inode!!!\n");
}

//释放一个inode的内存，根目录不能释放
static void put_free_inode(inode_t *inode){
    assert(inode != inode_table);
    assert(inode->count == 0);
    inode->dev = EOF;
}

//获取根目录的inode
inode_t *get_root_inode(){
    return inode_table;
}

//计算inode nr对应的块号(第nr个inode在第几个块)，？减一是因为从1开始
static inline idx_t inode_block(super_block_t *sb, idx_t nr){
    return 2 + sb->desc->imap_blocks + sb->desc->zmap_blocks + (nr - 1) / BLOCK_INODES;
}

//从已经打开的inodes中查找编号为nr的inode
static inode_t *find_inode(dev_t dev, idx_t nr){
    super_block_t *sb = get_super(dev);
    assert(sb);
    list_t *list = &sb->inode_list;

    for (list_node_t *node = list->head.next; node != &list->tail; node = node->next)
    {
        inode_t *inode = element_entry(inode_t, node, node);
        if (inode->nr == nr)
        {
            return inode;
        }
    }
    return NULL;
}

//获取设备dev的第nr个inode
inode_t *iget(dev_t dev, idx_t nr){
    inode_t *inode = find_inode(dev, nr);
    if (inode)
    {
        inode->count++;
        inode->atime = time();
        return inode;
    }
    
    //若inode尚未被打开，申请一个新的，并初始化
    super_block_t *sb = get_super(dev);
    
    assert(sb);
    assert(nr <= sb->desc->inodes);

    inode = get_free_inode();
    inode->dev = dev;
    inode->nr = nr;
    inode->count = 1;

    //加入链表
    list_push(&sb->inode_list, &inode->node);

    //读入缓冲
    idx_t block = inode_block(sb, inode->nr);
    buffer_t *buf = bread(inode->dev, block);
    inode->buf = buf;

    //将缓冲视作inode描述符的数组，获取对应的指针
    inode->desc = &((inode_desc_t *)buf->data)[(inode->nr - 1) % BLOCK_INODES];

    inode->ctime = time();
    inode->atime = time();

    return inode;
}

//释放inode
void iput(inode_t *inode){
    if (!inode)
    {
        return;
    }

    inode->count--;
    if (inode->count)
    {
        return;
    }
    
    //引用全都释放了，该释放inode了，先释放缓冲
    brelse(inode->buf);
    //从超级块的链表中移除
    list_remove(&inode->node);
    //释放inode内存
    put_free_inode(inode);
}

void inode_init(){
    for (size_t i = 0; i < INODE_NR; i++)
    {
        inode_t *inode = &inode_table[i];
        inode->dev = EOF;
    }
    
}

