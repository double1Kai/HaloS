#ifndef HALOS_FS_H
#define HALOS_FS_H

#include <halos/types.h>
#include <halos/list.h>
#include <halos/buffer.h>

//块大小
#define BLOCK_SIZE 1024
//扇区大小
#define SECTOR_SIZE 512
//文件系统魔数
#define MINIX1_MAGIC 0x137f
//文件名长度，单位字节
#define NAME_LEN 14

//inode位图的最大块数
#define IMAP_NR 8
//块位图的最大块数
#define ZMAP_NR 8

//块位图大小
#define BLOCK_BITS (BLOCK_SIZE * 8)

//每块inode描述符的数量
#define BLOCK_INODES (BLOCK_SIZE / sizeof(inode_desc_t))
//每块目录的数量
#define BLOCK_DENTRIES (BLOCK_SIZE / sizeof(dentry_t));
//每块索引的数量
#define BLOCK_INDEXS (BLOCK_SIZE / sizeof(u16))

//直接块数量
#define DIRECT_BLOCK (7)
//一级间接块数量
#define INDIRECT1_BLOCK BLOCK_INDEXS            
//二级间接块数量
#define INDIRECT2_BLOCK (INDIRECT1_BLOCK * INDIRECT1_BLOCK)
//全部块数量
#define TOTAL_BLOCK (DIRECT_BLOCK + INDIRECT1_BLOCK + INDIRECT2_BLOCK)

//inode描述
typedef struct inode_desc_t
{
    u16 mode;       //文件类型和属性
    u16 uid;        //用户id（文件拥有者的标识符）
    u32 size;       //文件大小，以字节为单位
    u32 mtime;      //修改时间戳，使用UCT时间
    u8 gid;         //组id，文件拥有者所在的组
    u8 nlinks;      //链接数，即有多少个文件目录指向该节点
    u16 zone[9];    //直接0~6，间接7，双重间接8，逻辑块号，u16意味着每次间接能多64个块
} inode_desc_t;

//inode
typedef struct inode_t
{
    inode_desc_t *desc;     //inode描述符
    struct buffer_t *buf;   //inode描述符对应的buffer
    dev_t dev;              //设备号
    idx_t nr;               //i节点号
    u32 count;              //引用计数
    time_t atime;           //访问时间
    time_t ctime;           //创建时间
    list_node_t node;      //链表节点
    dev_t mount;            //安装设备
}inode_t;

//超级块描述
typedef struct super_desc_t
{
    u16 inodes;     //有多少个inode
    u16 zones;      //有多少个文件块
    u16 imap_blocks;//inode的位图占用了多少块
    u16 zmap_blocks;//块的位图占用了多少块
    u16 firstdatazone;  //第一个数据逻辑块号，数据块的起始
    u16 log_zone_size;  //log2(文件块数)，一个zone占据多少块
    u32 max_size;       //文件最大长度，7+512+512*512 K
    u16 magic;          //文件系统魔数
} super_desc_t;

//超级块
typedef struct super_block_t
{
    super_desc_t *desc;     //超级块描述符
    struct buffer_t *buf;   //超级块描述符对应的buffer
    struct buffer_t *imaps[IMAP_NR];   //inode位图缓冲
    struct buffer_t *zmaps[ZMAP_NR];   //块位图缓冲
    dev_t dev;              //设备号
    list_t inode_list;//使用中（打开）的inode链表
    inode_t *iroot;          //根目录的inode
    inode_t *imount;          //TODO:文件系统的目录还能再挂载一个文件系统，之后再说
}super_block_t;

//文件目录结构
typedef struct dentry_t{
    u16 nr; //inode的索引
    char name[NAME_LEN]; //文件名
} dentry_t;

//获取dev对应的super块
super_block_t *get_super(dev_t dev);
//读取dev对应的super块
super_block_t *read_super(dev_t dev);

//分配一个文件块
idx_t balloc(dev_t dev);
//释放一个文件块
void bfree(dev_t dev, idx_t idx);
//分配一个inode
idx_t ialloc(dev_t dev);
//释放一个inode
void ifree(dev_t dev, idx_t idx);

//获取根目录的inode
inode_t *get_root_inode();
//获取设备dev的第nr个inode
inode_t *iget(dev_t dev, idx_t nr);
//释放inode
void iput(inode_t *inode);

//获取inode的第block块的索引，如果不存在且create为true，则创建
idx_t bmap(inode_t *inode, idx_t block, bool create);




//判断文件名是否相等
bool match_name(const char *name, const char *entry_name, char **next);
//获取dir目录下的name目录所在的dentry_t和buffer_t
buffer_t *find_entry(inode_t **dir, const char *name, char **next, dentry_t *result);
//在dir目录中添加name目录项
buffer_t *add_entry(inode_t *dir, const char *name, dentry_t **result);


#endif