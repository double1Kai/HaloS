# 编译
源文件（.c .cpp .h）->预处理[gcc -E]（.c）->编译[gcc -S]（.asm .s）->汇编[as]（.o .so）->链接[ld]（.out）
gcc集成了这些过程
# gcc汇编分析
## CFI
Call Frame Information /调用栈帧信息，用于调试，获取调用异常  
`-fno-asynchronous-unwind-tables`可以去掉 
## PIC
Positon Independent Code /位置无关代码
` __x86.get_pc_thunk.ax`
获取调用时`eip`的值，再通过`eip`的值得到`$_GLOBAL_OFFSET_TABLE_`，这里面存储了符号的地址信息  
从而生成位置无关代码  
`-fno-pic`可以去掉位置无关代码
## ident
GCC的版本信息  
`-Qn`可以去掉
## 栈对齐
使访问更加高效 
`` 