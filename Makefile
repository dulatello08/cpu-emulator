CC = gcc
CFLAGS = -Wall -Wextra -Werror -I. -g -O0
LIBS =

ifdef USE_UNIX_SOCKET
    CFLAGS += -DUSE_UNIX_SOCKET
    LIBS += -lsocket
endif

all: emulator

emulator: main.o emulator.o utilities.o execute_instructions.o mmu.o peripherals.o $(if $(USE_UNIX_SOCKET),socket/unix_socket.o)
	$(CC) $(CFLAGS) $^ $(LIBS) -o emulator

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

socket/unix_socket.o: socket/unix-socket.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -f *.o emulator socket/*.o

.PHONY: all clean