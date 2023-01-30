;读入内核加载器loader，并跳转至loader执行
[org 0x7c00];引导扇区起始，定了编译时的偏移地址，最后编译出来的二进制指令中的地址全部会加上0x7c00

;设置屏幕为文本模式，并清空屏幕
mov ax, 3;
int 0x10

;初始化寄存器
mov ax, 0
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7c00

mov si, booting;传参
call print

mov edi, 0x1000;写入的内存地址
mov ecx, 2;起始扇区
mov bl, 4;扇区数量，二者在makefile中决定

call read_disk

cmp word[0x1000], 0x55aa
jnz error

jmp 0:0x1002

jmp $

read_disk:
    mov dx, 0x1f2;扇区数量端口
    mov al, bl
    out dx, al;输入输出只能用ax

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
        call .reads;读取一个扇区的数据
        pop cx
        loop .read
    ret

    .waits:
        mov dx, 0x1f7;获取硬盘数据准备情况
        .check:
            in al, dx
            jmp $+2
            jmp $+2
            jmp $+2;延迟
            and al, 0b1000_1000;取第0位和第7位
            cmp al, 0b0000_1000;如果第7位是0，第三位是1，则正常返回
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

booting:
    db "HaloS is Booting...", 10, 13, 0;分别为\n\r结束

error:
    mov si, .msg
    call print
    hlt;停止Cpu
    jmp $
    .msg: 
        db "Booting Error!",10,13,0

;将当前地址至510字节用0填满
times 510 - ($ - $$) db 0
;最后填上55 aa，主引导扇区的结束标志 
db 0x55, 0xaa 