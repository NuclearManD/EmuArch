
#include <stdio.h>
#include <stdlib.h>
#include "argparse.h"
#include "memory.h"
#include "emulator.h"

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
}

int main(int argc, char** argv){
	char*	dual_ops[] = {"-c", "-mem"};
	char*	codefile_name;
	void*	code_data;
	int i;
	int memory_size = 64;
	t_emuarch_cpu* cpu;

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
	run(cpu);
}
