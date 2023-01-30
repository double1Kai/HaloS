#include<halos/printk.h>
#include<halos/console.h>
#include<halos/stdio.h>

static char buf[1024];

int printk(const char *fmt, ...){//format中有多少个百分号就有多少个额外参数
    va_list args;
    int i;
    va_start(args, fmt);
    i = vsprintf(buf, fmt, args);//将转化后的字符串放到buf中
    va_end(args);
    console_write(buf, i);
    return i; 
}
