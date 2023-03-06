#include <halos/bitmap.h>
#include <halos/string.h>
#include <halos/assert.h>
#include <halos/debug.h>

//构造位图
void bitmap_make(bitmap_t *map, char *bits, u32 length, u32 offset){
    map->bits = bits;
    map->length = length;
    map->offset = offset;
}
//位图初始化
void bitmap_init(bitmap_t *map, char *bits, u32 length, u32 offset){
    memset(bits, 0, length);
    bitmap_make(map, bits, length, offset);
}
//测试位图的某一位是否为1
bool bitmap_test(bitmap_t *map,u32 index){
    assert(index >= map->offset);
    //获取位图中的索引
    idx_t idx = index - map->offset;
    //位图中的字节数
    u32 bytes = idx / 8;
    //字节中的哪一位
    u32 bits = idx % 8;
    assert(bytes < map->length);
    return (map->bits[bytes] & (1 << bits));
}
//设置位图某位的值
void bitmap_set(bitmap_t *map, u32 index, bool value){
    assert(index >= map->offset);
    //获取位图中的索引
    idx_t idx = index - map->offset;
    //位图中的字节数
    u32 bytes = idx / 8;
    //字节中的哪一位
    u32 bits = idx % 8;
    assert(bytes < map->length);
    if(value){
        map->bits[bytes] |= (1 << bits);
    }else{
        map->bits[bytes] &= ~(1 << bits);
    }
}
//从位图中得到连续的count位未占用
int bitmap_scan(bitmap_t *map, u32 count){
    int start = EOF; //开始的位置
    u32 bits_left = map->length * 8; //剩余的位数
    u32 next_bit = 0; //下一个数
    u32 counter = 0; //计数器

    //从头开始找
    while (bits_left-- >0)
    {
        if(!bitmap_test(map,map->offset + next_bit)){
            //如果下一位未被占用，则counter++
            counter++;
        }else{
            counter = 0;
        }

        next_bit++;
        if(counter == count){
            start = next_bit - count;
            break;
        }
    }
    if(start == EOF){
        return EOF;
    }
    //将使用的位全部置为1
    bits_left = count;
    next_bit = start;
    while (bits_left--)
    {
        bitmap_set(map, map->offset + next_bit, true);
        next_bit++;
    }
    return start + map->offset;
}