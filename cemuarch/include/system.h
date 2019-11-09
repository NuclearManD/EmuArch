
#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>

#define SYSCALL_PUTCHAR 2

void syscall(t_emuarch_cpu* cpu, int16_t call_num);

#endif
