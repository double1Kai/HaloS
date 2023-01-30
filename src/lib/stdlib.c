#include <halos/stdlib.h>

//延迟
void delay(u32 count){
    while (count--)
        ;
}

//阻塞
void hang(){
    while (true)
        ;
}

//BCD码转整数 0x90 转 90
u8 bcd_to_bin(u8 value){
    return (value & 0xf) + (value >> 4) * 10;
}
//整数转BCD码
u8 bin_to_bcd(u8 value){
    return (value / 10) * 0x10 + (value % 10);
}
//计算num分成大小为size所占用的数量，除了之后得进位
u32 div_round_up(u32 num, u32 size){
    return (num + size -1) / size;
}