
#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

#define CODE_OFFSET	0x00000000
#define RAM_OFFSET	0x80000000
#define MAX_RAM_SIZE RAM_OFFSET

void setup_memory(void* code, int ram_size);

int8_t	ram_read_byte(int64_t address);
int16_t	ram_read_word(int64_t address);
int32_t	ram_read_dword(int64_t address);
int64_t	ram_read_qword(int64_t address);
int64_t	ram_read_size(int64_t address, char size);

void	ram_write_byte(int64_t address, int8_t data);
void	ram_write_word(int64_t address, int16_t data);
void	ram_write_dword(int64_t address, int32_t data);
void	ram_write_qword(int64_t address, int64_t data);
void	ram_write_size(int64_t address, int64_t data, char size);

#endif
