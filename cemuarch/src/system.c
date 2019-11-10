
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include "emulator.h"
#include "system.h"
#include "memory.h"

uint64_t micros(){
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return (1000000*tv.tv_sec) + tv.tv_usec;
}

uint64_t millis(){
	return micros()/1000;
}

int64_t heap_top = RAM_OFFSET;

void emuarch_syscall(t_emuarch_cpu* cpu, int16_t id){
	int64_t tmp;
	char*	tmp_str;

	if (id == SYSCALL_PUTCHAR){
		write(1, &(cpu->reg_set_0[0]), 1);
	}else if (id == SYSCALL_MALLOC){
		tmp = cpu->reg_set_0[0];
		cpu->reg_set_0[0] = heap_top;
		heap_top += tmp;
	}else if (id == SYSCALL_FREE){
		
	}else if (id == SYSCALL_PRINTF){
		// si is format string
		// arguments used are poped off the stack
		// to prevent stack corruption and/or undefined behavior, 
		//   a qword is popped for each argument, regardless of argument size.
		tmp_str = adr_to_str(cpu->reg_set_0[4]);
		while (*tmp_str){
			if (*(tmp_str) == '%'){
				tmp_str++;
				switch(*(tmp_str++)){
					case 'i':
						printf("%lli", pop_qword(cpu));
						break;
					case 's':
						printf("%s", adr_to_str(pop_qword(cpu)));
						break;
					case 0:
						return;
					default:
						printf("\nhmm = %c\n", *(tmp_str - 1));
						break;

				}
			}else
				printf("%c", *(tmp_str++));
		}

	}else if (id == SYSCALL_PUTSTR){
		printf("%s", adr_to_str(cpu->reg_set_0[4]));
	}else if (id == SYSCALL_MILLIS){
		cpu->reg_set_0[0] = millis();
	}else if (id == SYSCALL_MICROS){
		cpu->reg_set_0[0] = micros();
	}
}
