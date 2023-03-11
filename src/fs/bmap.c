#include <halos/fs.h>
#include <halos/debug.h>
#include <halos/bitmap.h>
#include <halos/assert.h>
#include <halos/string.h>
#include <halos/buffer.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

//分配一个文件块
idx_t balloc(dev_t dev){
    super_block_t *sb = get_super(dev);
    assert(sb);

    buffer_t *buf = NULL;
    idx_t bit = EOF;
    bitmap_t map;

    for (size_t i = 0; i < ZMAP_NR; i++)
    {
        buf = sb->zmaps[i];
        assert(buf);

        //将这一片缓冲区作为位图，？减一是因为从1开始
        bitmap_make(&map, buf->data, BLOCK_SIZE, i * BLOCK_BITS + sb->desc->firstdatazone - 1);

        //从位图中找一块
        bit = bitmap_scan(&map, 1);
        if (bit != EOF)
        {   
            assert(bit < sb->desc->zones);
            buf->dirty = true;
            break;
        }
    }
    bwrite(buf);//TODO:耗费性能，找点更好的办法
    return bit;
}

//释放一个文件块
void bfree(dev_t dev, idx_t idx){
    super_block_t *sb = get_super(dev);
    assert(sb);
    assert(idx < sb->desc->zones);
    
    buffer_t *buf = NULL;
    bitmap_t map;

    for (size_t i = 0; i < ZMAP_NR; i++)
    {
        //跳过开始的块
        if(idx > BLOCK_BITS * (i +1)){
            continue;
        }

        buf = sb->zmaps[i];
        assert(buf);

        //将这一片缓冲区作为位图
        bitmap_make(&map, buf->data, BLOCK_SIZE, i * BLOCK_BITS + sb->desc->firstdatazone - 1);

        //将idx位对应的位置为0
        assert(bitmap_test(&map, idx));
        bitmap_set(&map, idx, 0);

        buf->dirty = true;
        break;
    }
    bwrite(buf);//TODO:耗费性能，找点更好的办法
}

//分配一个inode
idx_t ialloc(dev_t dev){
    super_block_t *sb = get_super(dev);
    assert(sb);

    buffer_t *buf = NULL;
    idx_t bit = EOF;
    bitmap_t map;

    for (size_t i = 0; i < IMAP_NR; i++)
    {
        buf = sb->imaps[i];
        assert(buf);

        //将这一片缓冲区作为位图
        bitmap_make(&map, buf->data, BLOCK_SIZE, i * BLOCK_BITS);

        //从位图中找一块
        bit = bitmap_scan(&map, 1);
        if (bit != EOF)
        {   
            assert(bit < sb->desc->zones);
            buf->dirty = true;
            break;
        }
    }
    bwrite(buf);//TODO:耗费性能，找点更好的办法
    return bit;
}

//释放一个inode
void ifree(dev_t dev, idx_t idx){
    super_block_t *sb = get_super(dev);
    assert(sb);
    assert(idx < sb->desc->zones);
    
    buffer_t *buf = NULL;
    bitmap_t map;

    for (size_t i = 0; i < IMAP_NR; i++)
    {
        //跳过开始的块
        if(idx > BLOCK_BITS * (i +1)){
            continue;
        }

        buf = sb->imaps[i];
        assert(buf);

        //将这一片缓冲区作为位图
        bitmap_make(&map, buf->data, BLOCK_SIZE, i * BLOCK_BITS);

        //将idx位对应的位置为0
        assert(bitmap_test(&map, idx));
        bitmap_set(&map, idx, 0);

        buf->dirty = true;
        break;
    }
    bwrite(buf);//TODO:耗费性能，找点更好的办法
}

idx_t bmap(inode_t *inode, idx_t block, bool create){
    assert(block >=0 && block < TOTAL_BLOCK);

    //数组索引
    u16 index = block;
    //数组
    u16 *array = inode->desc->zone;
    //缓冲区
    buffer_t *buf = inode->buf;

    //引用次数+1，因为inode不应该释放，但为了之后统一操作brelse，这里加个1
    buf->count += 1;

    //当前处理的级别
    int level = 0;
    //当前的子级别的块数量
    int divider = 1;

    //直接块
    if (block < DIRECT_BLOCK)
    {
        goto reckon;
    }
    
    //不是直接块
    block -=DIRECT_BLOCK;
    if (block < INDIRECT1_BLOCK)
    {
        index = DIRECT_BLOCK;//指向一级索引块
        level = 1;
        divider = 1;
        goto reckon;
    }

    //不是一级间接块
    block -= INDIRECT1_BLOCK;
    assert(block < INDIRECT2_BLOCK);
    index = DIRECT_BLOCK + 1;//指向二级索引块
    level = 2;
    divider = BLOCK_INDEXS;
    
reckon:
    for(;level >= 0; level--){
        //不存在且要创建
        if (!array[index] && create)
        {
            array[index] = balloc(inode->dev);
            buf->dirty = true;
        }
        brelse(buf);//就是这里会让buf->count--，主要是为了写回

        //如果level为0或者文件块不存在（如果要创建之前已经创建了），直接返回
        if (level == 0 || !array[index])
        {
            return array[index];
        }
        
        //level不为0，且要找的文件块存在，处理下一级索引
        buf = bread(inode->dev, array[index]);
        index = block / divider;
        block = block % divider;
        divider /= BLOCK_INDEXS;
        array = (u16 *)buf->data; 
    }
}