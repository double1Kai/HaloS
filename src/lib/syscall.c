#include <halos/syscall.h>

//有0个其他参数的系统调用
static _inline u32 _syscall0(u32 nr){
    u32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(nr));
    return ret;
}
//有1个其他参数的系统调用
static _inline u32 _syscall1(u32 nr, u32 arg){
    u32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(nr), "b"(arg));
    return ret;
}
//有2个其他参数的系统调用
static _inline u32 _syscall2(u32 nr, u32 arg1, u32 arg2){
    u32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(nr), "b"(arg1), "c"(arg2));
    return ret;
}
//有3个其他参数的系统调用
static _inline u32 _syscall3(u32 nr, u32 arg1, u32 arg2, u32 arg3){
    u32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(nr), "b"(arg1), "c"(arg2), "d"(arg3));
    return ret;
}

u32 test(){
    return _syscall0(SYS_NR_TEST);
}
void exit(int status){
    _syscall1(SYS_NR_EXIT, (u32)status);
}
pid_t fork(){
    return _syscall0(SYS_NR_FORK);
}
pid_t waitpid(pid_t pid, int32* status){
    _syscall2(SYS_NR_WAITPID, pid, (u32)status);
}
void yield(){
    _syscall0(SYS_NR_YIELD);
}
void sleep(u32 ms){
    _syscall1(SYS_NR_SLEEP, ms);
}
int32 brk(void *addr){
    return _syscall1(SYS_NR_BRK, (u32)addr);
}
int32 write(fd_t fd, char *buf, u32 len){
    return _syscall3(SYS_NR_WRITE, fd, (u32)buf, len);
}
pid_t get_pid(){
    return _syscall0(SYS_NR_GETPID);
}
pid_t get_ppid(){
    return _syscall0(SYS_NR_GETPPID);
}
time_t time(){
    return _syscall0(SYS_NR_TIME);
}