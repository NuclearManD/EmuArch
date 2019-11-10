
#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>

#define SYSCALL_MALLOC	0
#define SYSCALL_FREE	1
#define SYSCALL_PUTCHAR	2
#define SYSCALL_PRINTF	3
#define SYSCALL_PUTSTR	4
#define SYSCALL_MILLIS	5
#define SYSCALL_MICROS	6

uint64_t micros();
uint64_t millis();

void emuarch_syscall(t_emuarch_cpu* cpu, int16_t call_num);

#endif
