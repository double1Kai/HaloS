#ifndef HALOS_STDLIB_H
#define HALOS_STDLIB_H

#include <halos/types.h>

#define MAX(a, b) (a < b ? b : a)
#define MIN(a, b) (a < b ? a : b)


void delay(u32 count);
void hang();

u8 bcd_to_bin(u8 value);//BCD码转二进制
u8 bin_to_bcd(u8 value);//二进制转BCD码

u32 div_round_up(u32 num, u32 size);

#endif