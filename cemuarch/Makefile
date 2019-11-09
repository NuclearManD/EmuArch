CC = gcc
FLAGS = -Iinclude -Wall -Wextra -Werror -Wno-unused-result

testbench:
	mkdir -p bin
	mkdir -p build
	$(CC) -c $(FLAGS) -O3 src/main.c -o build/main.o
	$(CC) -c $(FLAGS) -O3 src/emulator.c -o build/emulator.o
	$(CC) -c $(FLAGS) -O3 src/argparse.c -o build/argparse.o
	$(CC) -c $(FLAGS) -O3 src/memory.c -o build/memory.o
	$(CC) -c $(FLAGS) -O3 src/system.c -o build/system.o
	$(CC) $(FLAGS) -O3 build/*.o -o bin/emu-emuarch
debug:
	mkdir -p bin
	mkdir -p build
	$(CC) -c $(FLAGS) -g src/main.c -o build/main.o
	$(CC) -c $(FLAGS) -g src/emulator.c -o build/emulator.o
	$(CC) -c $(FLAGS) -g src/argparse.c -o build/argparse.o
	$(CC) -c $(FLAGS) -g src/memory.c -o build/memory.o
	$(CC) -c $(FLAGS) -g src/system.c -o build/system.o
	$(CC) $(FLAGS) -g build/*.o -o bin/emu-emuarch
clean:
	rm -rf build
fclean: clean
	rm -rf bin
re: fclean testbench
