
#include <stdlib.h>
#include "memory.h"

#define DECODE_ADR(x) (((x) < RAM_OFFSET) ? (global_code + (x)) : (global_ram + (x) - RAM_OFFSET))

void* global_code;
void* global_ram;

void setup_memory(void* code, uint32_t ram_size){
	global_ram = malloc(ram_size);
	global_code = code;
}

int8_t		ram_read_byte(uint64_t address){
	return *(int8_t*)DECODE_ADR(address);
}

int16_t	ram_read_word(uint64_t address){
	return *(int16_t*)DECODE_ADR(address);
}

int32_t	ram_read_dword(uint64_t address){
	return *(int32_t*)DECODE_ADR(address);
}

int64_t	ram_read_qword(uint64_t address){
	return *(int64_t*)DECODE_ADR(address);
}

int64_t	ram_read_size(uint64_t address, char size){
	switch (size){
		case 0:
			return *(int64_t*)DECODE_ADR(address);
		case 1:
			return *(int32_t*)DECODE_ADR(address);
		case 2:
			return *(int16_t*)DECODE_ADR(address);
		case 3:
			return *(int8_t*)DECODE_ADR(address);
	}
	return -1;
}


void	ram_write_byte(uint64_t address, int8_t data){
	*(int8_t*)DECODE_ADR(address) = data;
}

void	ram_write_word(uint64_t address, int16_t data){
	*(int16_t*)DECODE_ADR(address) = data;
}

void	ram_write_dword(uint64_t address, int32_t data){
	*(int32_t*)DECODE_ADR(address) = data;
}

void	ram_write_qword(uint64_t address, int64_t data){
	*(int64_t*)DECODE_ADR(address) = data;
}

void	ram_write_size(uint64_t address, int64_t data, char size){
	switch (size){
		case 0:
			*(int64_t*)DECODE_ADR(address) = data;
			break;
		case 1:
			*(int32_t*)DECODE_ADR(address) = data;
			break;
		case 2:
			*(int16_t*)DECODE_ADR(address) = data;
			break;
		case 3:
			*(int8_t*)DECODE_ADR(address) = data;
	}
}
