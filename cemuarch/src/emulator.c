
#include <stdlib.h>
#include "emulator.h"
#include "memory.h"

#define PC	reg_set_0[7]
#define SP	reg_set_0[6]
#define SI	reg_set_0[5]
#define DI	reg_set_0[4]
#define CNT	reg_set_1[7]

#define GETREG(x, y) ((((y) & 0x18) == 0) ? (x)->reg_set_0[y & 7] : (x)->reg_set_1[y & 7])

#define REG_SIZE(x)			(((x) >> 3) & 1)
#define SIZE_TO_BYTES(x)	(1 << (3 - (x)))
#define SIZE_TO_MASK(x)		((1 << (8 * (4 - (x)))) - 1)
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


t_emuarch_cpu* make_cpu(int64_t pc, int64_t sp){
	t_emuarch_cpu* cpu = malloc(sizeof(t_emuarch_cpu));
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
	// TODO: Set flags in crt0 register here

	return a;
}

void throw_exception(t_emuarch_cpu* cpu, uint16_t exception_id){

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

int step(t_emuarch_cpu* cpu){
	uint8_t reg_raw;
	uint8_t reg1, reg2;
	uint8_t opcode;
	uint8_t size;
	int64_t tmp, tmp1, data;
	uint64_t address;

	// first fetch an opcode
	opcode = ram_read_byte(cpu->PC);
	size = (opcode >> 2) & 3;
	cpu->PC++;

	// instruction decoding and execution...
	if (opcode & 0x80){
		// 0b1xxxxxxx
		if (opcode & 0x40){
			// 0b11xxxxxx

		}else{
			// 0b10xxxxxx
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
}

void run(t_emuarch_cpu* cpu){

}