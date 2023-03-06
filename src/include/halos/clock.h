#ifndef HALOS_CLOCK_H
#define HALOS_CLOCK_H

void pit_init();
void clock_handler(int vector);
void clock_init();
void start_beep();

#endif