
#ifndef EMULATOR_H
#define EMULATOR_H

#include <stdint.h>

// I am not yet implementing support for floats
#define EMULATOR_FEATURES 0x00000

typedef struct s_emuarch_cpu{
	int64_t	reg_set_0[8];
	int32_t	reg_set_1[8];
	uint64_t total_operations;
}	t_emuarch_cpu;

t_emuarch_cpu* make_cpu(int64_t pc, int64_t sp);

int step(t_emuarch_cpu* cpu);
void run(t_emuarch_cpu* cpu);

#endif
