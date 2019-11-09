
#include <stdlib.h>
#include "memory.h"

#define DECODE_ADR(x) (((x) < RAM_OFFSET) ? (global_code + (x)) : (global_ram + (x) - RAM_OFFSET))

void* global_code;
void* global_ram;

void setup_memory(void* code, int ram_size){
	global_ram = malloc(ram_size);
	global_code = code;
}

int8_t		ram_read_byte(int64_t address){
	return *(int8_t*)DECODE_ADR(address);
}

int16_t	ram_read_word(int64_t address){
	return *(int16_t*)DECODE_ADR(address);
}

int32_t	ram_read_dword(int64_t address){
	return *(int32_t*)DECODE_ADR(address);
}

int64_t	ram_read_qword(int64_t address){
	return *(int64_t*)DECODE_ADR(address);
}

int64_t	ram_read_size(int64_t address, char size){
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


void	ram_write_byte(int64_t address, int8_t data){
	*(int8_t*)DECODE_ADR(address) = data;
}

void	ram_write_word(int64_t address, int16_t data){
	*(int16_t*)DECODE_ADR(address) = data;
}

void	ram_write_dword(int64_t address, int32_t data){
	*(int32_t*)DECODE_ADR(address) = data;
}

void	ram_write_qword(int64_t address, int64_t data){
	*(int64_t*)DECODE_ADR(address) = data;
}

void	ram_write_size(int64_t address, int64_t data, char size){
	switch (size){
		case 0:
			*(int64_t*)DECODE_ADR(address) = data;
		case 1:
			*(int32_t*)DECODE_ADR(address) = data;
		case 2:
			*(int16_t*)DECODE_ADR(address) = data;
		case 3:
			*(int8_t*)DECODE_ADR(address) = data;
	}
}