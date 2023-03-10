#ifndef HALOS_BUFFER_H
#define HALOS_BUFFER_H

#include <halos/types.h>
#include <halos/list.h>
#include <halos/mutex.h>

//块大小
#define BLOCK_SIZE 1024
//扇区大小
#define SECTOR_SIZE 512
//每个块占多少个扇区
#define BLOCK_SECS (BLOCK_SIZE / SECTOR_SIZE)

typedef struct buffer_t
{
    char *data;         //数据区
    dev_t dev;          //设备号
    idx_t block;        //块号
    int count;          //引用计数
    list_node_t hnode;  //哈希表拉链节点
    list_node_t rnode;  //缓冲节点
    lock_t lock;        //锁
    bool dirty;         //是否脏
    bool valid;         //是否有效
}buffer_t;

//获取dev block对应的缓冲
buffer_t *getblk(dev_t dev, idx_t block);

//读取dev设备的block块
buffer_t *bread(dev_t dev, idx_t block);

//写dev设备的block块
void bwrite(buffer_t *bf);

//释放缓冲
void brelse(buffer_t *bf);


#endif