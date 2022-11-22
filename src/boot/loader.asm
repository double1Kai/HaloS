[org 0x1000]

dw 0x55aa;用于判断错误

mov si, loading
call print

detect_memory:
    xor ebx, ebx;首次ebx置0
    mov ax, 0
    mov es, ax
    mov edi, ards_buffer;es:di为缓冲区
    mov edx, 0x534d4150;固定签名
    mov ecx, 20
    .next:
        mov eax, 0xe820;准备系统调用子功能
        int 0x15
        jc error;检测返回的CF
        add di, cx;指向下一个ARDS
        inc word [ards_count]
        cmp ebx, 0
        jnz .next;ebx不为0则标志检测未结束

        mov si, detecting
        call print

prepare_protect_mode:

    cli;关闭中断
    ;打开A20线
    in al, 0x92
    or al, 0x10
    out 0x92, al

    ;加载gdt
    lgdt [gdt_ptr]

    ;启动保护模式
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ;用跳转来刷新缓存
    jmp dword code_selector:protect_mode

print:
    mov ah, 0x0e
    .next:
        mov al, [si]
        cmp al, 0
        jz .done
        int 0x10
        inc si
        jmp .next
    .done:
        ret

    jmp prepare_protect_mode

loading:
    db "HaloS is Loading...", 10, 13, 0;分别为\n\r结束

detecting:
    db "Memory Detecting Success!", 10, 13, 0;分别为\n\r结束

error:
    mov si, .msg
    call print
    hlt;停止Cpu
    jmp $
    .msg: 
        db "Loading Error!",10,13,0

[bits 32]
protect_mode:
    ;初始化段寄存器
    mov ax, data_selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x10000;修改栈顶

    mov edi, 0x10000;写入的内存地址
    mov ecx, 10;起始扇区
    mov bl, 200;扇区数量，二者在makefile中决定

    call read_disk

    jmp dword code_selector:0x10000

    ud2;表示出错

    jmp $

read_disk:
    mov dx, 0x1f2;扇区数量端口
    mov al, bl
    out dx, al

    inc dx;起始扇区低8位
    mov al, cl
    out dx, al

    inc dx;起始扇区中8位
    shr ecx, 8
    mov al, cl
    out dx, al

    inc dx;起始扇区高8位
    shr ecx, 8
    mov al, cl
    out dx, al

    inc dx
    shr ecx, 8
    and cl, 0b1111;取低4位
    mov al, 0b1110_0000;主盘，LBA模式
    or al, cl
    out dx, al

    inc dx
    mov al, 0x20;读硬盘
    out dx, al

    xor ecx, ecx
    mov cl, bl;读写扇区数量，准备循环

    .read:
        push cx
        call .waits;等待数据准备完毕
        call .reads;读取数据
        pop cx
        loop .read

    ret

    .waits:
        mov dx, 0x1f7
        .check:
            in al, dx
            jmp $+2
            jmp $+2
            jmp $+2;延迟
            and al, 0b1000_1000;取第0位和第7位
            cmp al, 0b0000_1000;如果第7位是0，第三位是1
            jnz .check;否则重复check
        ret

    .reads:
        mov dx, 0x1f0;开始读入数据，每次一个字
        mov cx, 256;每个扇区256个字，准备循环
        .readw:
            in ax, dx
            jmp $+2
            jmp $+2
            jmp $+2;延迟
            mov [edi], ax;读入位置在edi的内存中
            add edi, 2
            loop .readw
        ret

code_selector equ (1<<3);PRL和TI全为0，将index左移3位就行
data_selector equ (2<<3)

memory_base equ 0;内存基地址
memory_limit equ ((1024*1024*1024*4)/(1024*4))-1;内存界限

gdt_ptr:
    dw (gdt_end-gdt_base) - 1
    dd gdt_base

gdt_base:
    dd 0,0;第0个全为0
gdt_code:
    dw memory_limit & 0xffff        ;段界限的0~15位
    dw memory_base & 0xffff         ;基地址0~15位
    db (memory_base >> 16) & 0xff   ;基地址16~23位
    ;在内存中，DPL最高，代码段，非依从，可读，未被访问过
    db 0b_1_00_1_1_0_1_0
    ;粒度为4k，32位，非64位，段界限16~19位
    db 0b_1_1_0_0_0000 | (memory_limit >> 16) & 0xf
    db (memory_base >> 24) & 0xff   ;基地址24~31位
gdt_data:
    dw memory_limit & 0xffff        ;段界限的0~15位
    dw memory_base & 0xffff         ;基地址0~15位
    db (memory_base >> 16) & 0xff   ;基地址16~23位
    ;在内存中，DPL最高，数据段，向上扩展，可写，未被访问过
    db 0b_1_00_1_0_0_1_0
    ;粒度为4k，32位，非64位，段界限16~19位
    db 0b_1_1_0_0_0000 | (memory_limit >> 16) & 0xf
    db (memory_base >> 24) & 0xff   ;基地址24~31位
gdt_end:

ards_count:
    dw 0
ards_buffer:
