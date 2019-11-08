
#include <stdlib.h>
#include "emulator.h"
#include "memory.h"

#define PC	reg_set_0[7]
#define SP	reg_set_0[6]
#define SI	reg_set_0[5]
#define DI	reg_set_0[4]
#define CNT	reg_set_1[7]


t_emuarch_cpu* make_cpu(int64_t pc, int64_t sp){
	t_emuarch_cpu* cpu = malloc(sizeof(t_emuarch_cpu));
	cpu->PC = pc;
	cpu->SP = sp;
}

void step(){

}
void run();