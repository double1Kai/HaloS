#include <halos/debug.h>
#include <halos/stdarg.h>
#include <halos/stdio.h>
#include <halos/printk.h>

static char buf[1024];

void debugk(char *file, int line, const char *fmt, ...){
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    printk("[%s] [%d] %s", file, line, buf);
}