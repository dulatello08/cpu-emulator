CC = gcc
CFLAGS = -Wall -Wextra -Werror -I. -g -O0

all: emulator

emulator: main.o emulator.o utilities.o scheduler.o execute_instructions.o mmu.o peripherals.o
	$(CC) $(CFLAGS) $^ -o emulator

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

emulator.o: emulator.c
	$(CC) $(CFLAGS) -c emulator.c -o emulator.o

utilities.o: utilities.c
	$(CC) $(CFLAGS) -c utilities.c -o utilities.o

scheduler.o: scheduler.c
	$(CC) $(CFLAGS) -c scheduler.c -o scheduler.o

execute_instructions.o: execute_instructions.c
	$(CC) $(CFLAGS) -c execute_instructions.c -o execute_instructions.o

mmu.o: mmu.c
	$(CC) $(CFLAGS) -c mmu.c -o mmu.o

peripherals.o: peripherals.c
	$(CC) $(CFLAGS) -c peripherals.c -o peripherals.o

clean:
	rm -f *.o emulator

.PHONY: all clean