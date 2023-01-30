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
#include <halos/clock.h>
#include <halos/time.h>
#include <halos/rtc.h>
#include <halos/memory.h>
extern void mapping_init();
extern void bitmap_tests();

void kernel_init(){

    memory_map_init();
    mapping_init();
    interrupt_init();
    // clock_init();
    // time_init();
    // rtc_init();

    bitmap_tests();


    //asm volatile("sti");
    hang();
    return;
}