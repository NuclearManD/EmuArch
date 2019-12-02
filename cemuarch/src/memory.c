
#include <stdlib.h>
#include "memory.h"

char* adr_to_str(t_emuarch_cpu* cpu, int64_t address){
	return (char*)DECODE_ADR(cpu, address);
}

int8_t		ram_read_byte(t_emuarch_cpu* cpu, uint64_t address){
	return *(int8_t*)DECODE_ADR(cpu, address);
}

int16_t	ram_read_word(t_emuarch_cpu* cpu, uint64_t address){
	return *(int16_t*)DECODE_ADR(cpu, address);
}

int32_t	ram_read_dword(t_emuarch_cpu* cpu, uint64_t address){
	return *(int32_t*)DECODE_ADR(cpu, address);
}

int64_t	ram_read_qword(t_emuarch_cpu* cpu, uint64_t address){
	return *(int64_t*)DECODE_ADR(cpu, address);
}

int64_t	ram_read_size(t_emuarch_cpu* cpu, uint64_t address, char size){
	switch (size){
		case 0:
			return *(int64_t*)DECODE_ADR(cpu, address);
		case 1:
			return *(int32_t*)DECODE_ADR(cpu, address);
		case 2:
			return *(int16_t*)DECODE_ADR(cpu, address);
		case 3:
			return *(int8_t*)DECODE_ADR(cpu, address);
	}
	return -1;
}


void	ram_write_byte(t_emuarch_cpu* cpu, uint64_t address, int8_t data){
	*(int8_t*)DECODE_ADR(cpu, address) = data;
}

void	ram_write_word(t_emuarch_cpu* cpu, uint64_t address, int16_t data){
	*(int16_t*)DECODE_ADR(cpu, address) = data;
}

void	ram_write_dword(t_emuarch_cpu* cpu, uint64_t address, int32_t data){
	*(int32_t*)DECODE_ADR(cpu, address) = data;
}

void	ram_write_qword(t_emuarch_cpu* cpu, uint64_t address, int64_t data){
	*(int64_t*)DECODE_ADR(cpu, address) = data;
}

void	ram_write_size(t_emuarch_cpu* cpu, uint64_t address, int64_t data, char size){
	switch (size){
		case 0:
			*(int64_t*)DECODE_ADR(cpu, address) = data;
			break;
		case 1:
			*(int32_t*)DECODE_ADR(cpu, address) = data;
			break;
		case 2:
			*(int16_t*)DECODE_ADR(cpu, address) = data;
			break;
		case 3:
			*(int8_t*)DECODE_ADR(cpu, address) = data;
	}
}
