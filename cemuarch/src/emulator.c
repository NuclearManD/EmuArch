
#include <stdlib.h>
#include <stdio.h>
#include "emulator.h"
#include "flow_and_shape.h"
#include "memory.h"
#include "system.h"
#include "strings.h"

#define GETREG(x, y) ((((y) & 0x18) == 0) ? (x)->reg_set_0[y] : (x)->reg_set_1[y & 7])

#define REG_SIZE(x)			(((x) >> 3) & 1)
#define SIZE_TO_BYTES(x)	(1 << (3 - (x)))
#define SIZE_TO_MASK(x)		(0xFFFFFFFFFFFFFFFFULL >> (64 - (8 << (3 - (x)))))
#define REG_BITS(x)			(((x) & 8) ? 32 : 64)
#define REG_BYTES(x)		((16 - ((x) & 8)) >> 1)

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
#define OP_LSH	0x1C
#define OP_RSH	0x1D
#define OP_RAS	0x1E

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
		case OP_RAS:
			a = a >> b;
			break;
		case OP_LSH:
			a = a << b;
			break;
		case OP_RSH:
			a = (int64_t)((uint64_t)a >> b);
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

int64_t sarg_alu(t_emuarch_cpu* cpu, uint8_t op, int64_t a){
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
	uint64_t content;

	if (regid & 8){
		regid &= 7;
		content = cpu->reg_set_1[regid] & (0xFFFFFFFF ^ SIZE_TO_MASK(size));
		cpu->reg_set_1[regid] = content | data;
	}else{
		content = cpu->reg_set_0[regid] & (0xFFFFFFFFFFFFFFFFUL ^ SIZE_TO_MASK(size));
		cpu->reg_set_0[regid] = data | content;
	}
}

/*

Function unessecary - do not use.

int64_t read_reg(t_emuarch_cpu* cpu, uint8_t size, uint8_t regid){
	return GETREG(cpu, regid) & SIZE_TO_MASK(size);
}

*/

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

void flow_ops(t_emuarch_cpu* cpu){
	uint8_t subop = ram_read_byte(cpu->PC);
	cpu->PC++;
	if (subop == 0x00){
		// flow @

		cpu->PC += 8;
		run_flow(cpu->reg_set_0, cpu->reg_set_1, ram_read_qword(cpu->PC - 8));
	}else if(subop == 0x01){
		// flow si

		run_flow(cpu->reg_set_0, cpu->reg_set_1, cpu->SI);
	}else{
		throw_exception(cpu, ERROR_INVALID_INSTRUCTION);
	}
}

void call_immediate(t_emuarch_cpu* cpu, uint64_t function){
	int64_t stack_ptr = cpu->reg_set_0[6];
	int i;

	push_qword(cpu, cpu->PC);
	cpu->PC = function;
	
	while (1){
		i = step(cpu);
		if (i == -1){
			break;
		}else if (cpu->reg_set_0[6] >= stack_ptr){
			if (cpu->reg_set_0[6] != stack_ptr)
				cpu->reg_set_0[6] = stack_ptr;
			break;
		}
	}
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
				}else if (tmp1 == 0x0B){
					// [FLOW OPS]
					flow_ops(cpu);
				}else if (tmp1 == 0x0C){
					// [MEM & STR OPS]
					memstr_ops(cpu);
				}else if (tmp1 == 0x1F){
					// halt
					return -1;
				}
			}else{
				// 0b110xxxxx
				if ((tmp1 & 0x13) == 0x00){
					// lod[s]
					size = (opcode >> 2) & 3;
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
				switch(tmp1){
					case 0:
						// syscall **
						emuarch_syscall(cpu, ram_read_word(cpu->PC));
						cpu->PC += 2;
						break;
					case 1:
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
						break;
					case 16:
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
						break;
					default:
						throw_exception(cpu, ERROR_INVALID_INSTRUCTION);
				}
			}
		}
	}else{
		// 0b0xxxxxxx
		if (opcode & 0x40){
			// 0b01xxxxxx
			// Mathematical operations
			switch(opcode & 0xF0){
				case 0x40:
					// 0b0100xxxx
					// Single argument register mathematics
					reg1 = ram_read_byte(cpu->PC) & 0x1F;
					cpu->PC++;
					load_reg(cpu, reg1, sarg_alu(cpu, opcode & 15, GETREG(cpu, reg1)));
					break;
				case 0x50:
					// 0b0101xxxx
					// Register - register dual argument mathematics
					reg_raw = ram_read_byte(cpu->PC);
					cpu->PC++;

					// NOTE:	THIS CODE WILL NOT SUPPORT FLOAT OPERATIONS.  IT DOES NOT CHECK FOR
					// 			FLOATING POINT ADD, SUB, MUL, OR DIV.
					reg1 = reg_raw >> 4;
					reg2 = reg_raw & 15;

					load_reg(cpu, reg1, alu(cpu, opcode & 31, GETREG(cpu, reg1), GETREG(cpu, reg2)));
					break;
				case 0x60:
					// 0b0110xxxx
					// Register - constant dual argument mathematics
					reg1 = ram_read_byte(cpu->PC) & 15;
					cpu->PC++;

					if (((opcode & 31) < 0x1C && (opcode & 31) > 0x17) || (opcode & 31) == 0x1F)
						throw_exception(cpu, ERROR_INVALID_INSTRUCTION);

					tmp = ram_read_size(cpu->PC, REG_SIZE(reg1));
					cpu->PC += REG_BYTES(reg1);

					load_reg(cpu, reg1, alu(cpu, (opcode & 31) | 16, GETREG(cpu, reg1), tmp));
					break;
				default:
					throw_exception(cpu, ERROR_INVALID_INSTRUCTION);
			}
		}else{
			// 0b00xxxxxx
			if (opcode & 0x20){
				// 0b001xxxxx
				if (opcode & 0x10){
					// 0b0011xxxx (0x3X)
					if ((opcode & 0xC0) == 0){
						tmp = opcode & 3;
						switch(tmp){
							case 0:
								tmp = !(cpu->CR0 & 4);
								break;
							case 1:
								tmp = cpu->CR0 & 4;
								break;
							case 2:
								tmp = cpu->CR0 & 1;
								break;
							case 3:
								tmp = cpu->CR0 & 2;
								break;
						}
						if (tmp)
							cpu->PC = ram_read_qword(cpu->PC);
						else
							cpu->PC += 8;
					}else
						throw_exception(cpu, ERROR_INVALID_INSTRUCTION);
				}else{
					// 0b0010xxxxx (0x2X)
					switch((opcode >> 2) & 3){
						case 0:
							// mov[s] r1i, [r2i + **]
							size = opcode & 3;
							reg_raw = ram_read_byte(cpu->PC);
							reg1 = reg_raw >> 4;
							reg2 = reg_raw & 15;
							cpu->PC++;

							address = ram_read_word(cpu->PC) + GETREG(cpu, reg2);
							cpu->PC += 2;
							write_reg(cpu, size, reg1, ram_read_size(address, size));
							break;
						case 1:
							// mov[s] [r2i + **], r1i
							size = opcode & 3;
							reg_raw = ram_read_byte(cpu->PC);
							reg1 = reg_raw >> 4;
							reg2 = reg_raw & 15;
							cpu->PC++;

							address = ram_read_word(cpu->PC) + GETREG(cpu, reg2);
							ram_write_size(address, GETREG(cpu, reg1), size);
							cpu->PC += 2;
							break;
						default:
							throw_exception(cpu, ERROR_INVALID_INSTRUCTION);
					}
				}
			}else{
				// 0b000xxxxx
				if (opcode & 0x10){
					// 0b0001xxxx (0x1X)
					size = (opcode >> 2) & 3;
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
							alu(cpu, OP_CMP, cpu->reg_set_0[0] & SIZE_TO_MASK(size), ram_read_size(cpu->PC, size));
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
					size = (opcode >> 2) & 3;
					reg_raw = ram_read_byte(cpu->PC);
					cpu->PC++;
					reg1 = ((opcode & 2) << 3) | (reg_raw >> 4);
					reg2 = ((opcode & 1) << 4) | (reg_raw & 15);
					write_reg(cpu, size, reg1, GETREG(cpu, reg2));
				}
			}
		}
	}
	return opcode;
}

void run(t_emuarch_cpu* cpu){
	while (step(cpu) >= 0);
}
