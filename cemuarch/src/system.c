
#include <stdio.h>
#include <stdint.h>
#include "emulator.h"
#include "system.h"
#include "memory.h"

int64_t heap_top = RAM_OFFSET;

void syscall(t_emuarch_cpu* cpu, int16_t id){
	int64_t tmp;

	if (id == SYSCALL_PUTCHAR){
		printf("%c", (char) cpu->reg_set_0[0]);
	}else if (id == SYSCALL_MALLOC){
		tmp = cpu->reg_set_0[0];
		cpu->reg_set_0[0] = heap_top;
		heap_top += tmp;
	}else if (id == SYSCALL_FREE){
		
	}else if (id == SYSCALL_PRINTF){
		
	}
}
