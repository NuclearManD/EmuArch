
#include <stdio.h>
#include <stdint.h>
#include "emulator.h"
#include "system.h"

void syscall(t_emuarch_cpu* cpu, int16_t id){
	if (id == SYSCALL_PUTCHAR)
		printf("%c", (char) cpu->reg_set_0[0]);
}
