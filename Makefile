CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -I. -g -O0

all: emulator

emulator: main.o emulator.o
	$(CC) $(CFLAGS) $^ -o emulator

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

emulator.o: emulator.c
	$(CC) $(CFLAGS) -c emulator.c -o emulator.o

clean:
	rm -f *.o emulator

.PHONY: all clean