# 任务及上下文

## 任务
线程、进程，就是一个执行流；
切换任务时要保存
- 程序入口地址
- 堆栈 - 内核栈
- 上下文（寄存器信息）

## ABI调用约定
Application Binary Interface
linux和unix使用 System V ABI
调用方保存：
- eax
- ecx
- edx
实现方保存：
- ebx
- esi
- edi
- ebp
- esp

## 内存分页
4G/4K = 1M页

栈底在最高地址

