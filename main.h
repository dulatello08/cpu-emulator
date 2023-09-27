#ifndef INC_8_BIT_CPU_EMULATOR_MAIN_H
#define INC_8_BIT_CPU_EMULATOR_MAIN_H

#endif //INC_8_BIT_CPU_EMULATOR_MAIN_H

#include <signal.h>
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <ctype.h>
#include <ncurses.h>

#define MEMORY 65536
#define EXPECTED_PROGRAM_WORDS 256
#define BLOCK_SIZE 4096
#define MAX_INPUT_LENGTH 1024

#define OP_NOP 0x00
#define OP_ADD 0x01
#define OP_SUB 0x02
#define OP_MUL 0x03
#define OP_ADM 0x04
#define OP_SBM 0x05
#define OP_MLM 0x06
#define OP_ADR 0x07
#define OP_SBR 0x08
#define OP_MLR 0x09
#define OP_CLZ 0x0A
#define OP_STO 0x0B
#define OP_STM 0x0C
#define OP_LDM 0x0D
#define OP_PSH 0x0E
#define OP_POP 0x0F
#define OP_BRN 0x10
#define OP_BRZ 0x11
#define OP_BRO 0x12
#define OP_BRR 0x13
#define OP_BNR 0x14
#define OP_HLT 0x15
#define OP_JSR 0x16
#define OP_OSR 0x17
#define OP_RSM 0x18
#define OP_RLD 0x19

#define FLAGS_SIZE 0x1
#define STACK_SIZE 0xff
#define MMU_CONTROL_SIZE 0x0004
#define PERIPHERAL_CONTROL_SIZE 0x8
#define FLASH_CONTROL_SIZE 0x1
#define FLASH_BLOCK_SIZE 0x1000

#define PROGRAM_MEMORY_START 0x0000
#define PROGRAM_MEMORY_SIZE(program_size) program_size
#define USABLE_MEMORY_START(program_size) (program_size)
#define USABLE_MEMORY_SIZE(program_size) (MEMORY - (FLAGS_SIZE + STACK_SIZE + MMU_CONTROL_SIZE + PERIPHERAL_CONTROL_SIZE + FLASH_CONTROL_SIZE + FLASH_BLOCK_SIZE + PROGRAM_MEMORY_SIZE(program_size)))

#define FLAGS_START (MEMORY - (FLAGS_SIZE + STACK_SIZE + MMU_CONTROL_SIZE + PERIPHERAL_CONTROL_SIZE + FLASH_CONTROL_SIZE + FLASH_BLOCK_SIZE))
#define STACK_START (MEMORY - (STACK_SIZE + MMU_CONTROL_SIZE + PERIPHERAL_CONTROL_SIZE + FLASH_CONTROL_SIZE + FLASH_BLOCK_SIZE))
#define MMU_CONTROL_START (MEMORY - (MMU_CONTROL_SIZE + PERIPHERAL_CONTROL_SIZE + FLASH_CONTROL_SIZE + FLASH_BLOCK_SIZE))
#define PERIPHERAL_CONTROL_START (MEMORY - (PERIPHERAL_CONTROL_SIZE + FLASH_CONTROL_SIZE + FLASH_BLOCK_SIZE))
#define FLASH_CONTROL_START (MEMORY - (FLASH_CONTROL_SIZE + FLASH_BLOCK_SIZE))
#define FLASH_BLOCK_START (FLASH_CONTROL_START + FLASH_CONTROL_SIZE)

#define IS_ADDRESS_IN_RANGE(address, region) ((address >= (region).startAddress) && (address < ((region).startAddress + (region).size)))

#define READ_PERIPHERAL_MMAP 0x1

#define LCD_WIDTH 32
#define LCD_HEIGHT 4

#define INTERRUPT_TABLE_SIZE 10

struct memory_block {
    uint16_t startAddress;
    uint16_t size;
};

typedef struct {
    struct memory_block programMemory;
    struct memory_block usableMemory;
    struct memory_block flagsBlock;
    struct memory_block stackMemory;
    struct memory_block mmuControl;
    struct memory_block peripheralControl;
    struct memory_block flashControl;
    struct memory_block currentFlashBlock;
} MemoryMap;


// Define a structure for the interrupt queue
typedef struct {
    uint8_t* elements; // Dynamically allocated array to store interrupt sources
    uint8_t top; // Index of the top element
} InterruptQueue;

// Define a structure for interrupt vectors
typedef struct {
    uint8_t source;
    uint16_t handler;
} InterruptVector;

typedef struct {
    // Memory map
    MemoryMap mm;
    // General-purpose registers + 16 is PC
    uint8_t* reg;
    uint16_t* pc;

    // flags in separate variables
    uint8_t* in_subroutine;
    bool mask_interrupt;

    // Memory
    uint8_t* memory;

    // ALU Flags register
    bool z_flag;
    bool v_flag;

    // Display
    char display[LCD_WIDTH][LCD_HEIGHT];

    // Interrupts
    InterruptVector i_vector_table[INTERRUPT_TABLE_SIZE];
    InterruptQueue* i_queue;
} CPUState;

typedef struct {
    char *program_file;
    char *flash_file;
    uint8_t *program_memory;
    uint8_t **flash_memory;
    FILE *fpf;
    uint8_t *shared_data_memory;
    CPUState *state;
    uint8_t *emulator_running;
    pid_t emulator_pid;
    uint8_t program_size;
    int flash_size;
} AppState;

int start(CPUState *state, size_t program_size, size_t flash_size, const uint8_t* program_memory, uint8_t** flash_memory, uint8_t* memory);
uint8_t load_program(const char *program_file, uint8_t **program_memory);
long load_flash(const char *flash_file, FILE *fpf, uint8_t ***flash_memory);

void destroyCPUState(CPUState *state);

uint8_t count_leading_zeros(uint8_t x);

bool execute_instruction(CPUState *state);

void increment_pc(CPUState *state, uint8_t opcode);

void add(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode);
void subtract(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode);
void multiply(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode);
uint8_t memory_access(CPUState *state, uint8_t reg, uint16_t address, uint8_t mode, uint8_t srcDest);

void setupMmap(CPUState *state, uint8_t program_size);
bool handleWrite(CPUState *state, uint16_t address, uint8_t value);

void mmuControl(CPUState *state, uint8_t value);

void pushStack(CPUState *state, uint8_t value);
uint8_t popStack(CPUState *state, uint8_t *out);

void clear_display(char display[LCD_WIDTH][LCD_HEIGHT]);
void print_display(char display[LCD_WIDTH][LCD_HEIGHT]);
void write_to_display(char display[LCD_WIDTH][LCD_HEIGHT], uint8_t data);

void handle_connection(int client_fd, CPUState *state, uint8_t *shared_data_memory);

void add_interrupt_vector(InterruptVector table[INTERRUPT_TABLE_SIZE], uint8_t index, uint8_t source, uint16_t handler);
uint16_t get_interrupt_handler(const InterruptVector table[INTERRUPT_TABLE_SIZE], uint8_t source);

void tty_mode(AppState *appState);
