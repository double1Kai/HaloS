#include <halos/halos.h>
#include <halos/types.h>
#include <halos/io.h>
#include <halos/string.h>
#include <halos/console.h>
#include <halos/stdarg.h>
#include <halos/printk.h>
#include <halos/assert.h>
#include <halos/debug.h>
#include <halos/global.h>
#include <halos/task.h>
#include <halos/interrupt.h>
#include <halos/stdlib.h>

void kernel_init(){

    console_init();
    gdt_init();
    interrupt_init();

    asm volatile(
        "sti"//开中断
    );

    u32 counter = 0;
    while (true)
    {
        DEBUGK("looping in kernel init %d...\n",counter++);
        delay(100000000);
    }
    return;
}