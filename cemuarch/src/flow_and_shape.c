
#include <stdint.h>
#include "memory.h"

void run_flow(int64_t*	regs0, int32_t*	regs1, int64_t address)
{
	uint16_t*	flow = (uint16_t*)DECODE_ADR(address);
	uint16_t	current;
	uint8_t		reg1, set;
	int64_t		a, b;

	while ((current = *flow) != 0xFC00){
		reg1 = current & 7;
		if ((set = (current >> 6) & 1) == 0){
			a = regs0[reg1];
			b = regs0[(current >> 3) & 7];
		} else {
			a = regs1[reg1];
			b = regs1[(current >> 3) & 7];
		}
		switch (current >> 7){
			case 0x00:
				a++;
				break;
			case 0x01:
				a--;
				break;
			case 0x02:
				a = -a;
				break;
			case 0x03:
				a = ~a;
				break;
			case 0x04:
				a += b;
				break;
			case 0x05:
				a -= b;
				break;
			case 0x06:
				a *= b;
				break;
			case 0x07:
				a /= b;
				break;
			case 0x08:
				a &= b;
				break;
			case 0x09:
				a |= b;
				break;
			case 0x0A:
				a ^= b;
				break;
			case 0x0B:
				a = a << b;
				break;
			case 0x0C:
				a = (int64_t) (((uint64_t)a) >> b);
				break;
			case 0x0D:
				a = a >> b;
				break;
			default:
				break;
		}
		if ((set = (current >> 6) & 1) == 0)
			regs0[reg1] = a;
		else
			regs1[reg1] = (int32_t)a;
		flow += 2;
	}
}