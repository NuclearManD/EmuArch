
#include <string.h>
#include <stddef.h>
#include "emulator.h"

char* strskip(char* str, char c){
	while (*str && *str == c)
		str++;
	return str;
}

char* strfind(char* str, char c){
	while (*str && *str == c)
		str++;
	return str;
}

void strfinditer(t_emuarch_cpu* cpu, uint64_t function){
	char* str = cpu->reg_set_0[4];
	char c = cpu->reg_set_0[0];
	
	push_qword(cpu, str);
	call_immediate(cpu, function);
	while (1){
		if ((str = strfind(str, c)) == 0)
			break;
		cpu->reg_set_0[4] = str;
		call_immediate(cpu, function);
	}
	cpu->reg_set_0[4] = pop_qword(cpu);
}
