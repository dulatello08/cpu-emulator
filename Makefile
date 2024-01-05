CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wpedantic -I. -g -O0 -I/opt/homebrew/include/SDL2 -D_THREAD_SAFE
LIBS = -lncurses -L/opt/homebrew/lib -lSDL2

# Source directories
SRC_DIR = .
SOCKET_DIR = socket
KEYBOARD_DIR = keyboard

# Object files
COMMON_OBJS = main.o emulator.o utilities.o execute_instructions.o mmu.o peripherals.o utilities_tty.o interrupts.o
SOCKET_OBJS = unix-socket.o
KEYBOARD_OBJS = keyboard-main.o

# Pattern rule for compiling .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Specific rules for socket and keyboard objects
$(SOCKET_OBJS): $(SOCKET_DIR)/unix-socket.c
	$(CC) $(CFLAGS) -c $< -o $@

$(KEYBOARD_OBJS): $(KEYBOARD_DIR)/main.c
	$(CC) $(CFLAGS) -c $< -o $@

# Targets
all: emulator emulator_socket

emulator: $(COMMON_OBJS) $(KEYBOARD_OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

emulator_socket: CFLAGS += -DEMULATOR_SOCKET
emulator_socket: $(COMMON_OBJS) $(SOCKET_OBJS) $(KEYBOARD_OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

# Clean
clean:
	rm -f *.o emulator emulator_socket keyboard_main

.PHONY: all clean