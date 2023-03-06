[bits 32]

section .text

global task_switch

;此时栈中压入了一个参数，即下一个任务next的起始地址
task_switch:
    push ebp
    mov ebp, esp;保存栈帧

    push ebx
    push esi
    push edi

    mov eax, esp
    and eax ,0xfffff000;当前任务的起始位置

    mov [eax], esp;当前任务的栈顶保存到任务起始位置的四个字节

    mov eax, [ebp + 8];获取栈中压入的next的起始地址
    mov esp, [eax];恢复next的栈顶

    pop edi;这里pop的就是next的值了
    pop esi
    pop ebx
    pop ebp;这里不用leave恢复esp的值

    ret;next程序入口返回到eip