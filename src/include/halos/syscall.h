#ifndef HALOS_SYSCALL_H
#define HALOS_SYSCALL_H

#include <halos/types.h>

typedef enum syscall_t{
    SYS_NR_TEST,
    SYS_NR_EXIT = 1,
    SYS_NR_FORK = 2,
    SYS_NR_WRITE = 4,
    SYS_NR_WAITPID = 7,
    SYS_NR_TIME = 13,
    SYS_NR_GETPID = 20,
    SYS_NR_BRK = 45,
    SYS_NR_GETPPID = 64,
    SYS_NR_YIELD = 158,
    SYS_NR_SLEEP = 162,
}syscall_t;

u32 test();
pid_t fork();
pid_t waitpid(pid_t pid, int32 *status);
void exit(int status);
time_t time();
void yield();
void sleep(u32 ms);
pid_t get_pid();
pid_t get_ppid();
int32 write(fd_t fd, char *buf, u32 len);
int32 brk(void *addr); 

#endif