CFLAGS:= -m32 #32位程序
# CFLAGS+= -fno-builtin #不需要gcc内置函数
# CFLAGS+= -nostdinc #不需要标准头文件 
CFLAGS+= -fno-pic #不需要位置无关的代码 
# CFLAGS+= -fno-pie #不需要位置无关的可执行程序 
# CFLAGS+= -nostdlib #不需要标准库 
# CFLAGS+= -fno-stack-protector #不需要栈保护
# CFLAGS9= -fomit-frame-pointer #不需要栈帧
CFLAGS+= -fno-asynchronous-unwind-tables #不需要调CFI信息
CFLAGS+= -mpreferred-stack-boundary=2 #不需要栈对齐
CFLAGS+= -Qn #去掉gcc版本信息
CFLAGS+= -masm=intel #汇编格式为intel
CFLAGS:=$(strip ${CFLAGS})

.PHONY: hello.s
hello.s: hello.c
	gcc $(CFLAGS) -S $< -o $@

.PHONY: params.s
params.s: params.c
	gcc $(CFLAGS) -S $< -o $@