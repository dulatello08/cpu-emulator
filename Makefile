CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2

all: main

main: main.c
	$(CC) $(CFLAGS) main.c -o main

clean:
	rm -f main

.PHONY: all clean