#include <halos/string.h>

//拷贝字符串
char *strcpy(char *dest, const char *src){
    char *ptr = dest;
    while (true)
    {
        *ptr++ =*src;
        if(*src++ == EOS){
            return dest;
        }
    }
}

//拷贝n个字符，防止缓冲区溢出
char *strncpy(char *dest, const char *src, size_t count){
    char *ptr = dest;
    size_t nr = 0;
    for (; nr < count; nr++)
    {
        *ptr++ = *src;
        if(*src++ == EOS){
            return dest;
        }
    }
    dest[count - 1] = EOS;
    return dest;
}

//拼接字符串
char *strcat(char *dest, const char *src){
    char *ptr = dest;
    while (*ptr != EOS)
    {
        ptr++;
    }
    while (true)
    {
        *ptr++ = *src;
        if(*src++ == EOS){
            return dest;
        }
    }
}

//获取字符串长度
size_t strlen(const char *str){
    char *ptr = (char *)str;
    while (*ptr != EOS)
    {
        ptr++;
    }
    return ptr-str;
}

//比较字符串大小，lefthandside，righthandside
int strcmp(const char *lhs, const char *rhs){
    while (*lhs == *rhs && *lhs != EOS && *rhs !=EOS)
    {
        lhs++;
        rhs++;
    }
    return *lhs < *rhs ? -1 : *lhs > *rhs;
}

//从左侧开始查找第一个匹配的字符的位置
char *strchr(const char *str, int ch){
    char *ptr = (char *)str;
    while (true)
    {
        if(*ptr == ch){
            return ptr;
        }
        if(*ptr++ == EOS){
            return NULL;
        }
    }
}

//从右侧开始查找第一个字符匹配的位置
char *strrchr(const char *str, int ch){
    char *last = NULL;
    char *ptr = (char *)str;
    while (true)
    {
        if(*ptr == ch){
            last =  ptr;
        }
        if(*ptr++ == EOS){
            return last;
        }
    }
}

//比较内存区域
int memcmp(const void *lhs, const void *rhs, size_t count){
    char *lptr = (char *)lhs;
    char *rptr = (char *)rhs;
    while ((count > 0) && *lptr == *rptr)
    {
        lptr++;
        rptr++;
        count--;
    }
    if (count == 0)
        return 0;
    return *lptr < *rptr ? -1 : *lptr > *rptr;
}

//设置内存区域值全为ch
void *memset(void *dest, int ch, size_t count){
    char *ptr = dest;
    while (count--)
    {
        *ptr++ =ch;
    }
    return dest;
    
}

//拷贝内存区域
void *memcpy(void *dest, const void *src, size_t count){
    char *ptr = dest;
    while (count--)
    {
        *ptr++ =*((char*)(src++));
    }
    return dest;
}

//在一段内存找到ch字符
void *memchr(const void *str, int ch, size_t count){
    char *ptr = (char *)str;
    while (count--)
    {
        if(*ptr == ch){
            return (void *)ptr;
        }
        ptr++;
    }
}
