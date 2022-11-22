#ifndef HALOS_STDARG_H
#define HALOS_STDARG_H

typedef char * va_list;

#define va_start(ap, v) (ap = (va_list)&v + sizeof(char *)) //将参数列表开始的指针放到va_list中
#define va_arg(ap, t) (*(t *)((ap += sizeof(char *)) - sizeof(char*))) //取出参数指针所指的一个类型为t的参数，并将指针指向下一个参数
#define va_end(ap) (ap = (va_list)0) //将参数指针置为空指针

#endif