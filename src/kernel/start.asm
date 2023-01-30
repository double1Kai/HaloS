[bits 32]

extern console_init
extern kernel_init
extern memory_init
extern gdt_init

global _start
_start:
    push ebx;给memory_init传参
    push eax
    call console_init
    call gdt_init
    call memory_init
    call kernel_init

    jmp $