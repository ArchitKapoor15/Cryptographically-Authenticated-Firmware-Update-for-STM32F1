#ifndef INC_SYSTEM_H
#define INC_SYSTEM_H

#include "common-defines.h"

#define SYS_FREQ 1000
#define CPU_FREQ 72000000

void system_setup(void);
uint64_t getTicks(void);
void system_delay(uint64_t millis);
void system_teardown(void);

#endif