
#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

#define CODE_OFFSET	0x00000000
#define RAM_OFFSET	0x80000000
#define MAX_RAM_SIZE RAM_OFFSET

void setup_memory(void* code, int ram_size);

uint8_t		ram_read_byte(uint64_t address);
uint16_t	ram_read_word(uint64_t address);
uint32_t	ram_read_dword(uint64_t address);
uint64_t	ram_read_qword(uint64_t address);
uint64_t	ram_read_size(uint64_t address, char size);

void	ram_write_byte(uint64_t address, uint8_t data);
void	ram_write_word(uint64_t address, uint16_t data);
void	ram_write_dword(uint64_t address, uint32_t data);
void	ram_write_qword(uint64_t address, uint64_t data);
void	ram_write_size(uint64_t address, uint64_t data, char size);

#endif
