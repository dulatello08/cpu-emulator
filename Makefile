CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wpedantic -I. -g -O0
LIBS =  -lncurses

all: emulator emulator_socket

emulator: main.o emulator.o utilities.o execute_instructions.o mmu.o peripherals.o interrupts.c
	$(CC) $(CFLAGS) $^ $(LIBS) -o emulator -lncurses

emulator_socket: CFLAGS += -DEMULATOR_SOCKET
emulator_socket: main.o emulator.o utilities.o execute_instructions.o mmu.o peripherals.o unix-socket.o interrupts.c
	$(CC) $(CFLAGS) $^ $(LIBS) -o emulator_socket

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

emulator.o: emulator.c
	$(CC) $(CFLAGS) -c emulator.c -o emulator.o

utilities.o: utilities.c
	$(CC) $(CFLAGS) -c utilities.c -o utilities.o

execute_instructions.o: execute_instructions.c
	$(CC) $(CFLAGS) -c execute_instructions.c -o execute_instructions.o

mmu.o: mmu.c
	$(CC) $(CFLAGS) -c mmu.c -o mmu.o

peripherals.o: peripherals.c
	$(CC) $(CFLAGS) -c peripherals.c -o peripherals.o

unix-socket.o: socket/unix-socket.c
	$(CC) $(CFLAGS) -c socket/unix-socket.c -o unix-socket.o

interrupts.o: interrupts.c
	$(CC) $(CFLAGS) -c interrupts.c -o interrupts.o

clean:
	rm -f *.o emulator emulator_socket

.PHONY: all clean emulator_socket