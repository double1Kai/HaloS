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
extern void task_init();
extern void syscall_init();
extern void keyboard_init();
extern void tss_init();
extern void ide_init();
extern void arena_init();
extern void buffer_init();
extern void super_init();

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

void kernel_init(){
    tss_init();
    memory_map_init();
    mapping_init();
    arena_init();
    
    interrupt_init();
    clock_init();
    keyboard_init();
    time_init();
    // rtc_init();
    ide_init();
    buffer_init();
    task_init();
    syscall_init();
    super_init();
    
    set_interrupt_state(true);
    return;
}