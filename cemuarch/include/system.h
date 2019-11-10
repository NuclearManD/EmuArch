
#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>

#define SYSCALL_MALLOC	0
#define SYSCALL_FREE	1
#define SYSCALL_PUTCHAR	2
#define SYSCALL_PRINTF	3

void syscall(t_emuarch_cpu* cpu, int16_t call_num);

#endif
