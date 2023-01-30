#ifndef HALOS_CLOCK_H
#define HALOS_CLOCK_H

#include <halos/io.h>
#include <halos/interrupt.h>
#include <halos/assert.h>
#include <halos/debug.h>
#include <halos/task.h>

void pit_init();
void clock_handler(int vector);
void clock_init();
void start_beep();

#endif