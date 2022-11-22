#include <halos/assert.h>
#include <halos/printk.h>
#include <halos/types.h>
#include <halos/stdarg.h>
#include <halos/stdio.h>

static u8 buf[1024];

//强制阻塞
static void spin(char *name){
    printk("spinning in %s ... \n", name);
    while (true)
        ;
}

void assertion_failure(char* exp, char *file, char* base, int line){
    printk(
        "\n--> assert(%s) faild!!!\n"
        "--> file: %s \n"
        "--> base: %s \n"
        "--> line: %d \n",
        exp, file, base, line
    );

    spin("assertion_failure()");

    //不可能到这里，否则出错
    asm volatile("ud2");//汇编代码表示出错
}

void panic(const char *fmt, ...){
    va_list args;
    va_start(args, fmt);
    int i = vsprintf(buf, fmt, args);
    va_end(args);

    printk("!!! panic !!!\n--> %s \n", buf);
    spin("panic()");

    //不可能到这里，否则出错
    asm volatile("ud2");//汇编代码表示出错
}