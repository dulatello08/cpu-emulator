UNAME_S := $(shell uname -s)

CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wpedantic -I. -g -O0 -D_THREAD_SAFE
LIBS =

ifeq ($(UNAME_S),Linux)
    CFLAGS += -I/usr/include/SDL2
    LIBS += -lSDL2
endif
ifeq ($(UNAME_S),Darwin)
    CFLAGS += -I/opt/homebrew/include/SDL2
    LIBS += -L/opt/homebrew/lib -lSDL2
endif

# Source directories
SRC_DIR = .
SOCKET_DIR = socket
KEYBOARD_DIR = keyboard
GUI_DIR = gui

# Object files
COMMON_OBJS = main.o emulator.o utilities.o execute_instructions.o mmu.o peripherals.o utilities_sdl.o interrupts.o
SOCKET_OBJS = $(SOCKET_DIR)/unix_socket.o
KEYBOARD_OBJS = $(KEYBOARD_DIR)/main.o $(KEYBOARD_DIR)/scan_code_map.o
GUI_OBJS = $(GUI_DIR)/main.o

# Pattern rule for compiling .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Specific rules for socket and keyboard objects
$(SOCKET_DIR)/%.o: $(SOCKET_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(KEYBOARD_DIR)/%.o: $(KEYBOARD_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(GUI_DIR)/%.o: $(GUI_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Targets
all: emulator gui_subsystem

emulator: $(COMMON_OBJS) $(KEYBOARD_OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

emulator_socket: CFLAGS += -DEMULATOR_SOCKET
emulator_socket: $(COMMON_OBJS) $(SOCKET_OBJS) $(KEYBOARD_OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

gui_subsystem: $(GUI_OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

# Clean
clean:
	rm -f $(COMMON_OBJS) $(SOCKET_OBJS) $(KEYBOARD_OBJS) $(GUI_OBJS) $(GUI_DIR)/gui_subsystem emulator emulator_socket

.PHONY: all clean