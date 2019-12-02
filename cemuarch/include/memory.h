
#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

#include "emulator.h"

#define CODE_OFFSET	0x00000000
#define RAM_OFFSET	0x80000000
#define MAX_RAM_SIZE RAM_OFFSET

#define DECODE_ADR(c,x) (((x) < RAM_OFFSET) ? (c->code + (x)) : (c->ram + (x) - RAM_OFFSET))

char* adr_to_str(t_emuarch_cpu* cpu, int64_t address);

int8_t	ram_read_byte(t_emuarch_cpu* cpu, uint64_t address);
int16_t	ram_read_word(t_emuarch_cpu* cpu, uint64_t address);
int32_t	ram_read_dword(t_emuarch_cpu* cpu, uint64_t address);
int64_t	ram_read_qword(t_emuarch_cpu* cpu, uint64_t address);
int64_t	ram_read_size(t_emuarch_cpu* cpu, uint64_t address, char size);

void	ram_write_byte(t_emuarch_cpu* cpu, uint64_t address, int8_t data);
void	ram_write_word(t_emuarch_cpu* cpu, uint64_t address, int16_t data);
void	ram_write_dword(t_emuarch_cpu* cpu, uint64_t address, int32_t data);
void	ram_write_qword(t_emuarch_cpu* cpu, uint64_t address, int64_t data);
void	ram_write_size(t_emuarch_cpu* cpu, uint64_t address, int64_t data, char size);

#endif
