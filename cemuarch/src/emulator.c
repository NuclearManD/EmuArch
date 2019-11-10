
#include <stdlib.h>
#include <stdio.h>
#include "emulator.h"
#include "memory.h"
#include "system.h"

#define PC	reg_set_0[7]
#define SP	reg_set_0[6]
#define SI	reg_set_0[4]
#define DI	reg_set_0[5]
#define CNT	reg_set_1[6]
#define CR0	reg_set_1[7]

#define GETREG(x, y) ((((y) & 0x18) == 0) ? (x)->reg_set_0[y & 7] : (x)->reg_set_1[y & 7])

#define REG_SIZE(x)			(((x) >> 3) & 1)
#define SIZE_TO_BYTES(x)	(1 << (3 - (x)))
#define SIZE_TO_MASK(x)		(0xFFFFFFFFFFFFFFFFULL >> (64 - (8 << (3 - (x)))))
#define REG_BITS(x)			(((x) & 8) ? 32 : 64)

#define ERROR_INVALID_INSTRUCTION -100

#define OP_INC	0x00
#define OP_DEC	0x01
#define OP_NEG	0x02
#define OP_NOT	0x03
#define OP_SQRT	0x04
#define OP_TANH	0x05

#define OP_ADD	0x10
#define OP_SUB	0x11
#define OP_MUL	0x12
#define OP_DIV	0x13
#define OP_AND	0x14
#define OP_OR	0x15
#define OP_XOR	0x16
#define OP_CMP	0x17

/*
Here is a old-fashioned pecan pie recipe for you :
Ingredients
- Pastry dough
- 3/4 stick unsalted butter
- 1 1/4 cups packed light brown sugar
- 3/4 cup light corn syrup
- 2 teaspoon pure vanilla extract
- 1/2 teaspoon grated orange zest
- 1/4 teaspoon salt
- 3 large eggs
- 2 cups pecan halves (1/2 pound)
Accompaniment: whipped cream or vanilla ice cream
Preparation:
Preheat oven to 350°F with a baking sheet on middle rack.
Roll out dough on a lightly floured surface with a lightly floured rolling pin
into a 12 inch round and fit into a 9 inch pie plate.
Trim edge, leaving a 1/2-inch overhang.
Fold overhang under and lightly press against rim of pie plate, then crimp
decoratively.
Lightly prick bottom all over with a fork.
Chill until firm, at least 30 minutes (or freeze 10 minutes).
Meanwhile, melt butter in a small heavy saucepan over medium heat.
Add brown sugar, whisking until smooth.
Remove from heat and whisk in corn syrup, vanilla, zest, and salt.
Lightly beat eggs in a medium bowl, then whisk in corn syrup mixture.
Put pecans in pie shell and pour corn syrup mixture evenly over them.
Bake on hot baking sheet until filling is set, 50 minutes to 1 hour.
Cool completely.
Cooks notes:
Pie can be baked 1 day ahead and chilled. Bring to room temperature before
serving.

Source: 42 Silicon Valley, C Piscine Rush02 subject
*/

t_emuarch_cpu* make_cpu(int64_t pc, int64_t sp){
	t_emuarch_cpu* cpu = malloc(sizeof(t_emuarch_cpu));
	cpu->total_operations = 0;
	cpu->PC = pc;
	cpu->SP = sp;
	return cpu;
}

int64_t alu(t_emuarch_cpu* cpu, uint8_t op, int64_t a, int64_t b){
	switch (op){
		case OP_INC:
			a++;
			break;
		case OP_DEC:
			a--;
			break;
		case OP_NEG:
			a = -a;
			break;
		case OP_NOT:
			a = ~a;
			break;
		case OP_SQRT:
			; // NOT YET IMPLEMENTED
		case OP_TANH:
			; // NOT YET IMPLEMENTED
		case OP_XOR:
			a ^= b;
			break;
		case OP_OR:
			a |= b;
			break;
		case OP_AND:
			a &= b;
			break;
		case OP_MUL:
			a *= b;
			break;
		case OP_DIV:
			a = a / b;
			break;
		case OP_ADD:
			a += b;
			break;
		case OP_SUB:
		case OP_CMP:
			a -= b;
		default:
			break;
	}
	
	cpu->CR0 &= 0xFF00;
	cpu->CR0 |= EMULATOR_FEATURES;
	if (a > 0)
		cpu->CR0 |= 2;
	else if(a < 0)
		cpu->CR0 |= 1;
	else
		cpu->CR0 |= 4;

	return a;
}

void throw_exception(t_emuarch_cpu* cpu, uint16_t exception_id){
	
	// TODO: Add interrupt support
	cpu->CNT = exception_id;
	cpu->PC = 0x10;
}

void load_reg(t_emuarch_cpu* cpu, uint8_t regid, int64_t data){
	if (regid & 8)
		cpu->reg_set_1[regid & 7] = data;
	else
		cpu->reg_set_0[regid] = data;
}

void write_reg(t_emuarch_cpu* cpu, uint8_t size, uint8_t regid, int64_t data){
	uint64_t content = GETREG(cpu, regid);
	
	content &= (0xFFFFFFFFFFFFFFFFUL ^ SIZE_TO_MASK(size));
	content |= data;

	load_reg(cpu, regid, content);
}

int64_t read_reg(t_emuarch_cpu* cpu, uint8_t size, uint8_t regid){
	return GETREG(cpu, regid) & SIZE_TO_MASK(size);
}

int64_t pop_qword(t_emuarch_cpu* cpu){
	int64_t data = ram_read_qword(cpu->SP + 1);
	cpu->SP += 8;
	return data;
}

int64_t pop_size(t_emuarch_cpu* cpu, uint8_t size){
	int64_t data = ram_read_size(cpu->SP + 1, size);
	cpu->SP += SIZE_TO_BYTES(size);
	return data;
}

void push_size(t_emuarch_cpu* cpu, int64_t data, uint8_t size){
	uint64_t adr = cpu->SP -= SIZE_TO_BYTES(size);
	ram_write_size(adr + 1, data, size);
}

void push_qword(t_emuarch_cpu* cpu, int64_t data){
	uint64_t adr = cpu->SP -= 8;
	ram_write_qword(adr + 1, data);
}

int step(t_emuarch_cpu* cpu){
	uint8_t reg_raw;
	uint8_t reg1, reg2;
	uint8_t opcode;
	uint8_t size;
	int64_t tmp, tmp1, data;
	uint64_t address;
	uint64_t mask;

	// first fetch an opcode
	opcode = ram_read_byte(cpu->PC);
	//printf("%08lX > %02hhX\n", cpu->PC, opcode);
	size = (opcode >> 2) & 3;
	cpu->PC++;
	
	// increase instruction count counter
	cpu->total_operations++;

	// instruction decoding and execution...
	if (opcode & 0x80){
		// 0b1xxxxxxx
		tmp1 = opcode & 0x1F;
		if (opcode & 0x40){
			// 0b11xxxxxx
			if (opcode & 0x20){
				// 0b111xxxxx
				if (tmp1 == 0){
					// jmp @
					cpu->PC = ram_read_qword(cpu->PC);
				}else if (tmp1 == 0x1F){
					// halt
					return -1;
				}
			}else{
				// 0b110xxxxx
				if ((tmp1 & 0x13) == 0x00){
					// lod[s]
					write_reg(cpu, size, 0, ram_read_size(cpu->SI, size));
					cpu->SI+=SIZE_TO_BYTES(size);
				}
			}
		}else{
			// 0b10xxxxxx
			if (opcode & 0x20){
				// 0b101xxxxx
				// CISC stack ops
				if (tmp1 == 0){
					// pop r1
					reg1 = ram_read_byte(cpu->PC) & 0x1F;
					cpu->PC++;
					
					load_reg(cpu, reg1, pop_size(cpu, REG_SIZE(reg1)));
				}else if (tmp1 == 1){
					// push r1
					reg1 = ram_read_byte(cpu->PC) & 0x1F;
					cpu->PC++;
					
					push_size(cpu, GETREG(cpu, reg1), REG_SIZE(reg1));
				}else if (tmp1 == 2){
					// call @
					push_qword(cpu, cpu->PC + 8);
					cpu->PC = ram_read_qword(cpu->PC);
				}
			}else{
				// 0b100xxxxx
				if (tmp1 == 0){
					// syscall **
					syscall(cpu, ram_read_word(cpu->PC));
					cpu->PC += 2;
				}else if (tmp1 == 1){
					// mov[s] <msb reverse bit> r1, ?
					reg_raw = ram_read_byte(cpu->PC);
					cpu->PC++;
					
					reg1 = reg_raw & 0x1F;
					size = (reg_raw >> 5) & 3;
					mask = SIZE_TO_MASK(size);
					
					data = ram_read_size(cpu->PC, size);
					cpu->PC += SIZE_TO_BYTES(size);
					
					if (reg_raw & 128){
						tmp1 = REG_BITS(reg1) >> 1;
						data = data << (tmp1);
						mask = mask << (tmp1);
						//printf("  foof\n");
					}
					
					mask ^= SIZE_TO_MASK(REG_SIZE(reg1));
					//printf("  %016llX OR ((%016llX XOR %016llX) AND %016llX) -> reg %i\n", 
					//	data, SIZE_TO_MASK(REG_SIZE(reg1)), SIZE_TO_MASK(size), GETREG(cpu, reg1), reg1);
					load_reg(cpu, reg1, (GETREG(cpu, reg1) & mask) | data);
				}else if (tmp1 == 16){
					// j[c] r1, @
					reg_raw = ram_read_byte(cpu->PC);
					
					tmp = GETREG(cpu, reg_raw & 0x0F);
					
					if ((reg_raw >> 4) == 0){
						// jz r1, @
						if (tmp == 0)
							cpu->PC = ram_read_qword(cpu->PC + 1);
						else
							cpu->PC += 9;
					}else{
						// jnz r1, @
						if (tmp != 0)
							cpu->PC = ram_read_qword(cpu->PC + 1);
						else
							cpu->PC += 9;
					}
				}
			}
		}
	}else{
		// 0b0xxxxxxx
		if (opcode & 0x40){
			// 0b01xxxxxx
			// Mathematical operations
			tmp1 = opcode >> 4;
			if (tmp1 == 4){
				// 0b0100xxxx
				// Single argument register mathematics
				reg1 = ram_read_byte(cpu->PC) & 0x1F;
				cpu->PC++;
				load_reg(cpu, reg1, alu(cpu, opcode & 15, GETREG(cpu, reg1), 0));
			}else if (tmp1 == 5){
				// 0b0101xxxx
				// Register - register dual argument mathematics
				reg_raw = ram_read_byte(cpu->PC);
				cpu->PC++;

				// NOTE:	THIS CODE WILL NOT SUPPORT FLOAT OPERATIONS.  IT DOES NOT CHECK FOR
				// 			FLOATING POINT ADD, SUB, MUL, OR DIV.
				reg1 = reg_raw >> 4;
				reg2 = reg_raw & 15;

				load_reg(cpu, reg1, alu(cpu, opcode & 31, GETREG(cpu, reg1), GETREG(cpu, reg2)));
			}else if (tmp1 == 6){
				// 0b0110xxxx
				// Register - constant dual argument mathematics
				reg1 = ram_read_byte(cpu->PC) & 15;
				cpu->PC++;

				if (((opcode & 31) < 0x1C && (opcode & 31) > 0x17) || (opcode & 31) == 0x1F)
					throw_exception(cpu, ERROR_INVALID_INSTRUCTION);

				tmp = ram_read_size(cpu->PC, REG_SIZE(reg1));
				cpu->PC += REG_BITS(reg1) >> 3;

				load_reg(cpu, reg1, alu(cpu, (opcode & 31) | 16, GETREG(cpu, reg1), tmp));
			}else{
				throw_exception(cpu, ERROR_INVALID_INSTRUCTION);
			}
		}else{
			// 0b00xxxxxx
			if (opcode & 0x20){
				// 0b001xxxxx
				if (opcode & 0x10){
					// 0b0011xxxx (0x3X)
				}else{
					// 0b0010xxxxx (0x2X)
					size = opcode & 3;
					if (size == 0){
						// mov[s] r1i, [r2i + **]
						reg_raw = ram_read_byte(cpu->PC);
						reg1 = reg_raw >> 4;
						reg2 = reg_raw & 15;
						cpu->PC++;

						address = ram_read_word(cpu->PC) + GETREG(cpu, reg2);
						cpu->PC += 2;
						write_reg(cpu, size, reg1, ram_read_size(address, size));
					}else if (size == 1){
						// mov[s] [r2i + **], r1i
						reg_raw = ram_read_byte(cpu->PC);
						reg1 = reg_raw >> 4;
						reg2 = reg_raw & 15;
						cpu->PC++;

						address = ram_read_word(cpu->PC) + GETREG(cpu, reg2);
						ram_write_size(address, GETREG(cpu, reg1), size);
						cpu->PC += 2;
					}else{
						throw_exception(cpu, ERROR_INVALID_INSTRUCTION);
					}
				}
			}else{
				// 0b000xxxxx
				if (opcode & 0x10){
					// 0b0001xxxx (0x1X)
					switch(size){
						case 0:
							// exx r1, r2
							reg_raw = ram_read_byte(cpu->PC);
							cpu->PC++;
							reg1 = ((opcode & 2) << 3) | (reg_raw >> 4);
							reg2 = ((opcode & 1) << 4) | (reg_raw & 15);
							data = GETREG(cpu, reg2);
							tmp = GETREG(cpu, reg1);
							load_reg(cpu, reg2, tmp);
							load_reg(cpu, reg1, data);
							break;
						case 1:
							// cmp[s] rax, ?
							size = opcode & 3;
							alu(cpu, OP_CMP, cpu->reg_set_0[0] & SIZE_TO_MASK(size), ram_read_size(size, cpu->PC));
							cpu->PC += SIZE_TO_BYTES(size);
							break;
						case 2:
							// l32 r, #
							write_reg(cpu, 1, opcode & 3, ram_read_dword(cpu->PC));
							cpu->PC += 4;
							break;
						default:
							throw_exception(cpu, ERROR_INVALID_INSTRUCTION);
					}
				}else{

				}
			}
		}
	}
	return opcode;
}

void run(t_emuarch_cpu* cpu){
	while (step(cpu) >= 0);
}
