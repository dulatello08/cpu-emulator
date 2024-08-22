#ifndef INC_8_BIT_CPU_EMULATOR_MAIN_H
#define INC_8_BIT_CPU_EMULATOR_MAIN_H

#include <stdint.h>
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
#define OP_ENI 0x1A
#define OP_DSI 0x1B
#define OP_LSH 0x1C
#define OP_LSR 0x1D
#define OP_RSH 0x1E
#define OP_RSR 0x1F
#define OP_AND 0x20
#define OP_ORR 0x21
#define OP_MULL 0x22
#define OP_XOR 0x23

#define FLAGS_SIZE 0x2
// flags 1 is reserved, 2 is to rerender display
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
#define INTERRUPT_QUEUE_MAX 10

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
    uint8_t* sources; // Dynamically allocated array to store interrupt sources
    uint8_t* size; // Index of the top element
} InterruptQueue;

typedef struct {
    uint8_t sources[10];
    uint8_t size; // Index of the top element
} GuiInterruptQueue;

//gui shared memory type
typedef struct {
    char display[LCD_WIDTH][LCD_HEIGHT];
    uint8_t keyboard_o[2];
    GuiInterruptQueue i_queue;
} gui_process_shm_t;

// Define a structure for interrupt vectors
typedef struct {
    uint8_t source;
    uint16_t handler;
} InterruptVectors;

typedef struct {
    // Memory map
    MemoryMap mm;
    // General-purpose registers + 16 is PC
    uint8_t* reg;
    uint16_t* pc;

    // flags in separate variables
    bool enable_mask_interrupts;

    // Memory
    uint8_t* memory;

    // ALU Flags register
    bool z_flag;
    bool v_flag;

    // Display
    char display[LCD_WIDTH][LCD_HEIGHT];

    // Interrupts
    InterruptVectors i_vector_table[INTERRUPT_TABLE_SIZE];
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
    size_t program_size;
    int flash_size;
    gui_process_shm_t *gui_shm;
    pid_t gui_pid;
    int gui_shm_fd;
} AppState;

int start(AppState *appState);
size_t load_program(const char *program_file, uint8_t **program_memory);
long load_flash(const char *flash_file, FILE *fpf, uint8_t ***flash_memory);

uint8_t count_leading_zeros(uint8_t x);

bool execute_instruction(CPUState *state);

void increment_pc(CPUState *state, uint8_t opcode);

void add(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode);
void subtract(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode);
void multiply(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode);
void left_shift(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode);
void right_shift(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode);
void bitwise_and(CPUState *state, uint8_t operand_rd, uint8_t operand_rn);
void bitwise_or(CPUState *state, uint8_t operand_rd, uint8_t operand_rn);
void bitwise_xor(CPUState *state, uint8_t operand_rd, uint8_t operand_rn);
uint8_t memory_access(CPUState *state, uint8_t reg, uint16_t address, uint8_t mode, uint8_t srcDest);
bool hasChanged(int* lastValue, int currentValue);

void setupMmap(CPUState *state, size_t program_size);
bool handleWrite(CPUState *state, uint16_t address, uint8_t value);

void mmuControl(CPUState *state, uint8_t value);

void pushStack(CPUState *state, uint8_t value);
uint8_t popStack(CPUState *state, uint8_t *out);

void clear_display(char display[LCD_WIDTH][LCD_HEIGHT]);
void print_display(char display[LCD_WIDTH][LCD_HEIGHT]);
void write_to_display(char display[LCD_WIDTH][LCD_HEIGHT], uint8_t data);

void handle_connection(int client_fd, CPUState *state, uint8_t *shared_data_memory);

void add_interrupt_vector(InterruptVectors table[INTERRUPT_TABLE_SIZE], uint8_t index, uint8_t source, uint16_t handler);
uint16_t get_interrupt_handler(const InterruptVectors table[INTERRUPT_TABLE_SIZE], uint8_t source);

void push_interrupt(InterruptQueue* queue, uint8_t source);
uint8_t pop_interrupt(InterruptQueue* queue);

void open_gui(AppState *appState);

#endif //INC_8_BIT_CPU_EMULATOR_MAIN_H
