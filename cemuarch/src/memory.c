
#include <stdlib.h>
#include "memory.h"

#define DECODE_ADR(x) (((x) < RAM_OFFSET) ? (global_code + (x)) : (global_ram + (x) - RAM_OFFSET))

void* global_code;
void* global_ram;

void setup_memory(void* code, int ram_size){
	global_ram = malloc(ram_size);
	global_code = code;
}

uint8_t		ram_read_byte(uint64_t address){
	return *(uint8_t*)DECODE_ADR(address);
}

uint16_t	ram_read_word(uint64_t address){
	return *(uint16_t*)DECODE_ADR(address);
}

uint32_t	ram_read_dword(uint64_t address){
	return *(uint32_t*)DECODE_ADR(address);
}

uint64_t	ram_read_qword(uint64_t address){
	return *(uint64_t*)DECODE_ADR(address);
}

uint64_t	ram_read_size(uint64_t address, char size){
	switch (size){
		case 0:
			return *(uint64_t*)DECODE_ADR(address);
		case 1:
			return *(uint32_t*)DECODE_ADR(address);
		case 2:
			return *(uint16_t*)DECODE_ADR(address);
		case 3:
			return *(uint8_t*)DECODE_ADR(address);
	}
	return -1;
}


void	ram_write_byte(uint64_t address, uint8_t data){
	*(uint8_t*)DECODE_ADR(address) = data;
}

void	ram_write_word(uint64_t address, uint16_t data){
	*(uint16_t*)DECODE_ADR(address) = data;
}

void	ram_write_dword(uint64_t address, uint32_t data){
	*(uint32_t*)DECODE_ADR(address) = data;
}

void	ram_write_qword(uint64_t address, uint64_t data){
	*(uint64_t*)DECODE_ADR(address) = data;
}

void	ram_write_size(uint64_t address, uint64_t data, char size){
	switch (size){
		case 0:
			*(uint64_t*)DECODE_ADR(address) = data;
		case 1:
			*(uint32_t*)DECODE_ADR(address) = data;
		case 2:
			*(uint16_t*)DECODE_ADR(address) = data;
		case 3:
			*(uint8_t*)DECODE_ADR(address) = data;
	}
}