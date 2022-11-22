#include <halos/global.h>
#include <halos/debug.h>
#include <halos/string.h>

descriptor_t gdt[GDT_SIZE];
pointer_t gdt_ptr;

//将全局描述符表换个位置,并扩充到128个
void gdt_init(){
    DEBUGK("init gdt...\n");

    asm volatile("sgdt gdt_ptr");//将loader中的指针放到gdt_ptr中

    memcpy(&gdt, (void*)gdt_ptr.base, gdt_ptr.limit+1);//将loader中的描述符表拷贝到gdt中

    gdt_ptr.base = (u32)&gdt;
    gdt_ptr.limit = sizeof(gdt) - 1;
    
    asm volatile("lgdt gdt_ptr\n");//将当前的指针加载到gdt寄存器中
}