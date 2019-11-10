
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "argparse.h"
#include "memory.h"
#include "emulator.h"
#include "system.h"

void* read_file(char* filename){
	void* filedata = NULL;
	int size;
	FILE* fp = fopen(filename, "r");

	if (fp != NULL){
		// Go to end of file to detect code length
		if (fseek(fp, 0L, SEEK_END) == 0) {
			size = ftell(fp);
			filedata = malloc(size);

			// go back to the beginning and read the whole file
			fseek(fp, 0L, SEEK_SET);
			fread(filedata, 1, size, fp);

		}
		fclose(fp);
	}

	return filedata;
}

void print_usage(char* program_name){
	printf("Usage: %s [options]\n", program_name);
	//printf(" > With invalid options or no options, this is printed.");
	printf("Options:\n");
	printf("\t-c [codefile]\t: Specify raw binary file of program to load. Required.\n");
	printf("\t-mem [RAM size]\t: Specify RAM size, in KiB.  Defaults to 64.\n");
	printf("\t-d\t\t\t: Use debug terminal.\n");
	printf("\t-s\t\t\t: Print stats after run.  Ignored with -d.\n");
}

int main(int argc, char** argv){
	char*			dual_ops[] = {"-c", "-mem", 0};
	char*			regnames[] = {
		"rax", "rbx", "rcx", "rdx",
		"si", "di", "sp", "pc",
		"r0", "r1", "r2", "r3", "r4", "r5", "cnt", "cr0"
	};
	char*			codefile_name;
	void*			code_data;
	int				i;
	char			c;
	int				memory_size = 64;
	int64_t			stack_ptr;
	uint64_t		us_timer;
	t_emuarch_cpu*	cpu;

	argsort(argc, argv, dual_ops);

	i = get_arg(argc, argv, "-c");
	if (i == -1 || i + 1 >= argc || argv[i + 1] == NULL){
		print_usage(argv[0]);
		return -100;
	}
	codefile_name = argv[i + 1];

	i = get_arg(argc, argv, "-mem");
	if (i != -1 && i + 1 < argc && argv[i + 1] != NULL)
		memory_size = atoi(argv[i + 1]);

	code_data = read_file(codefile_name);
	if (code_data == NULL){
		printf("Error reading %s; abort\n", codefile_name);
		return -404;
	}

	memory_size *= 1024;
	setup_memory(code_data, memory_size);
	cpu = make_cpu(0, (memory_size - 1) | RAM_OFFSET);

	if (get_arg(argc, argv, "-d") == -1){
		us_timer = micros();
		run(cpu);
		us_timer = micros() - us_timer;
		if (get_arg(argc, argv, "-s") != -1){
			printf("\n\nProgram statistics:\n");
			printf("  Operations executed: %lli\n", cpu->total_operations);
			printf("  Execution time (us): %lli\n", us_timer);
			printf("  Operations / second: %f Million\n", ((float)cpu->total_operations) / us_timer);
		}
	}else{
		printf("Running in debug mode.\n");
		printf("Type h for help.\n");
		while (1){
			printf("> ");
			fflush(stdout);
			fread(&c, 1, 1, stdin);
			fread(&i, 1, 1, stdin);
			if (c == 'h'){
				printf(" s : step CPU\n r : run CPU\n p : print registers\n d : data/statistics\n o : step over\n n : print next opcode\n");
			}else if (c == 's'){
				printf(" 0x%016llX | ", cpu->reg_set_0[7]);
				i = step(cpu);
				if (i != -1)
					printf(" Opcode 0x%02hhX executed.\n", (char)(0xFF & i));
				else
					printf(" CPU Halted : opcode 0xFF.\n");
			}else if (c == 'p'){
				printf(" CPU registers:\n");
				for (i = 0; i < 8; i++)
					printf("  Reg %s\t| 0x%016llX\n", regnames[i], cpu->reg_set_0[i]);
				for (i = 0; i < 8; i++)
					printf("  Reg %s\t| 0x%08X\n", regnames[i + 8], cpu->reg_set_1[i]);
			}else if (c == 'r'){
				run(cpu);
			}else if (c == 'd'){
				printf(" Total operations executed: %llu\n", cpu->total_operations);
			}else if (c == 'n'){
				printf(" 0x%016llX | 0x%02hhX\n", cpu->reg_set_0[7], ram_read_byte(cpu->reg_set_0[7]));
			}else if (c == 'o'){
				stack_ptr = cpu->reg_set_0[6];
				printf(" Stack pointer: 0x%016llX\n", stack_ptr);
				while (1){
					i = step(cpu);
					if (i == -1){
						printf(" Stop: CPU Halted.\n");
						break;
					}else if (cpu->reg_set_0[6] >= stack_ptr){
						if (cpu->reg_set_0[6] != stack_ptr)
							printf(" Warning: stack pointer no longer matches.\n  New value: 0x%016llX\n", cpu->reg_set_0[6]);
						break;
					}
				}
			}
		}
	}
}
