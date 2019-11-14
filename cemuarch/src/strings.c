
#include <string.h>
#include <stddef.h>
#include "emulator.h"
#include "memory.h"

#include <stdio.h>

int strskip(char* str, char c){
	int i = 0;

	while (str[i] == c && str[i])
		i++;
	return i;
}

int strfind(char* str, char c){
	int i = 0;

	while (str[i] != c && str[i] != 0)
		i++;
	if (str[i] == 0)
		return -1;
	return i;
}

void strfinditer(t_emuarch_cpu* cpu, uint64_t function){
	char* str = adr_to_str(cpu->SI);
	int64_t adr = cpu->SI;
	char c = cpu->reg_set_0[0];
	int i;

	push_qword(cpu, cpu->SI);
	
	// call for beginning of string
	call_immediate(cpu, function);
	while (*str){
		//printf("\n[%s] == [%s]\n", str, adr_to_str(cpu->SI));
		if ((i = strfind(str, c)) == -1)
			break;
		str += i + 1;
		cpu->SI = adr += i + 1;

		// call for string starting at each instance of the searched character
		call_immediate(cpu, function);
	}
	
	cpu->SI = pop_qword(cpu);
}

void memstr_ops(t_emuarch_cpu* cpu){
	uint64_t tmp64;
	uint8_t subop = ram_read_byte(cpu->PC);

	cpu->PC++;
	if (subop == 0x00){
		// strlen si

		cpu->reg_set_0[0] = strlen(adr_to_str(cpu->SI));
	}else if(subop == 0x08){
		// strcat di, si

		strcat(adr_to_str(cpu->DI), adr_to_str(cpu->SI));
	}else if(subop == 0x10){
		// strcpy di, si

		strcpy(adr_to_str(cpu->DI), adr_to_str(cpu->SI));
	}else if(subop == 0x18){
		// strf si, rax

		cpu->SI += strfind(adr_to_str(cpu->SI), cpu->reg_set_0[0]);
	}else if(subop == 0x20){
		// strcmp di, si

		cpu->reg_set_0[0] = strcmp(adr_to_str(cpu->DI), adr_to_str(cpu->SI));
	}else if(subop == 0x28){
		// strfi si, rax, @

		tmp64 = ram_read_qword(cpu->PC);
		cpu->PC += 8;
		strfinditer(cpu, tmp64);
	}else if(subop == 0x30){
		// strskip si, rax

		cpu->SI += strskip(adr_to_str(cpu->SI), cpu->reg_set_0[0]);
	}else{
		throw_exception(cpu, ERROR_INVALID_INSTRUCTION);
	}
	
}
