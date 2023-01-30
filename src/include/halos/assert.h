#ifndef HALOS_ASSERT_H
#define HALOS_ASSERT_H

void assertion_failure(char* exp, char *file, char* base, int line);

#define assert(exp) \
    if(exp) \
        ; \
    else \
        assertion_failure(#exp, __FILE__,__BASE_FILE__,__LINE__)

void panic(const char *fmt, ...);//系统运行不下去了，直接报错

#endif