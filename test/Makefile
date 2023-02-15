CC = gcc
CFLAGS = -Wall -Wextra -Werror -I. -g -O0

all: test

test: main.o
	$(CC) $(CFLAGS) $^ -o test

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o