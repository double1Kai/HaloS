[bits 32]
section .text;代码段

global inb;将inb导出
inb:
    push ebp
    mov ebp, esp;保存栈帧

    xor eax, eax
    mov edx, [ebp + 8];获取port，存到edx中
    in al, dx;将端口号为dx的8位输入到al

    jmp $+2;延迟
    jmp $+2
    jmp $+2

    leave ;恢复栈帧
    ret

global outb;将outb导出
outb:
    push ebp
    mov ebp, esp;保存栈帧 

    mov edx, [ebp + 8];port
    mov eax, [ebp + 12];value
    out dx, al;

    jmp $+2
    jmp $+2
    jmp $+2

    leave ;恢复栈帧
    ret

global inw;将inw导出
inw:
    push ebp
    mov ebp, esp;保存栈帧

    xor eax, eax
    mov edx, [ebp + 8];获取port
    in ax, dx;将端口dx的8位输入al

    jmp $+2
    jmp $+2
    jmp $+2

    leave ;恢复栈帧
    ret

global outw;将outwb导出
outw:
    push ebp
    mov ebp, esp;保存栈帧 

    mov edx, [ebp + 8];port
    mov eax, [ebp + 12];value
    out dx, ax;

    jmp $+2
    jmp $+2
    jmp $+2

    leave ;恢复栈帧
    ret