#ifndef HALOS_STDIO_H
#define HALOS_STDIO_H

#include<halos/stdarg.h>

int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);
int printf(const char *buf, ...);

#endif