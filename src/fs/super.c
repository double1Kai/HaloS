#include <halos/fs.h>
#include <halos/buffer.h>
#include <halos/device.h>
#include <halos/assert.h>
#include <halos/string.h>
#include <halos/debug.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

//超级块的数目，相当于支持的文件系统数
#define SUPER_NR 16

//超级块表
static super_block_t super_table[SUPER_NR];
//根文件系统的超级块
static super_block_t *root;

//从超级块表中找个空闲的
static super_block_t *get_free_super(){
    for (size_t i = 0; i < SUPER_NR; i++)
    {
        super_block_t *sb = &super_table[i];
        if (sb->dev == EOF){
            return sb;
        }
    }
    panic("no more super block!!!\n");
}

//获取dev对应的super块
super_block_t *get_super(dev_t dev){
    for (size_t i = 0; i < SUPER_NR; i++)
    {
        super_block_t *sb = &super_table[i];
        if (sb->dev == dev)     
        {
            return sb;
        }
    }
    return NULL;
}

//读取dev对应的super块
super_block_t *read_super(dev_t dev){
    super_block_t *sb = get_super(dev);
    if (sb)
    {
        return sb;
    }
    LOGK("Reading super block of device %d ...\n", dev);

    sb = get_free_super();

    //读取对应的超级块
    buffer_t *buf = bread(dev, 1);

    sb->buf = buf;
    sb->desc = (super_desc_t *)buf->data;
    sb->dev = dev;

    assert(sb->desc->magic == MINIX1_MAGIC);

    memset(sb->imaps, 0, sizeof(sb->imaps));
    memset(sb->zmaps, 0, sizeof(sb->zmaps));

    //读取inode位图
    int idx = 2;//第0块引导块，第一块超级块，inode从第二块开始

    for(int i = 0; i < sb->desc->imap_blocks; i++){
        assert(i < IMAP_NR);
        //右侧不为0时，才会执行
        if (sb->imaps[i] = bread(dev, idx)){
            idx++;
        }else{
            break;
        }
    }

    //读取块位图
    for(int i = 0; i < sb->desc->zmap_blocks; i++){
        assert(i < ZMAP_NR);
        if (sb->zmaps[i] = bread(dev, idx)){
            idx++;
        }else{
            break;
        }
    }

    return sb;
}

//挂载根文件系统
static void mount_root(){
    LOGK("Mount root file system...\n");
    //默认主盘的第一个分区是根文件系统
    device_t *device = device_find(DEV_IDE_PART, 0);
    assert(device);
    //读根文件系统超级块
    root = read_super(device->dev); 

    root->iroot = iget(device->dev, 1); //获取根目录的inode
    root->imount = iget(device->dev, 1); //根目录挂载inode

    idx_t idx = 0;
    inode_t *inode = iget(device->dev,1);

    //直接块
    idx = bmap(inode, 3, true);
    //一级间接块
    idx = bmap(inode, 7 + 7, true);
    //二级间接块
    idx = bmap(inode, 7 + 512 * 3 + 510, true);

    iput(inode);
}

void super_init(){
    for (size_t i = 0; i < SUPER_NR; i++)
    {
        super_block_t *sb = &super_table[i];
        sb->dev = EOF;
        sb->desc = NULL;
        sb->buf = NULL;
        sb->iroot = NULL;
        sb->imount = NULL;
        list_init(&sb->inode_list);
    }

    mount_root();
}

