#ifndef HALOS_TYPES_H
#define HALOS_TYPES_H

#include <halos/halos.h>

#define EOF -1//End Of File
#define NULL ((void* )0)//空指针
#define EOS '\0'

#ifndef __cplusplus
#define bool _Bool
#define true 1
#define false 0
#endif //为了兼容C++编译器

#define _packed __attribute__((packed))//定义特殊的结构体,无边界对齐
#define _ofp __attribute__((optimize("omit-frame-pointer")))//省略函数的栈帧
#define _inline __attribute__((always_inline)) inline

typedef unsigned int size_t;

typedef char int8; 
typedef short int16;
typedef int int32;
typedef long long int64;

typedef unsigned char u8; 
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef u32 time_t;
typedef u32 idx_t;

#endif
