#ifndef HALOS_CONSOLE_H
#define HALOS_CONSOLE_H

#include<halos/types.h>

void console_init();
void console_clear();
void console_write(char *buf, u32 count);

#endif