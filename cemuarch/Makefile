CC = gcc
FLAGS = -Iinclude -Wall -Wextra -Werror -O3

testbench:
	mkdir -p bin
	mkdir -p build
	$(CC) -c $(FLAGS) src/main.c -o build/main.o
	$(CC) -c $(FLAGS) src/emulator.c -o build/emulator.o
	$(CC) -c $(FLAGS) src/argparse.c -o build/argparse.o
	$(CC) -c $(FLAGS) src/memory.c -o build/memory.o
	#$(CC) -c $(FLAGS) src/system.c -o build/system.o
	$(CC) $(FLAGS) build/*.o -o bin/emu-emuarch
clean:
	rm -rf build
fclean: clean
	rm -rf bin
re: fclean testbench
