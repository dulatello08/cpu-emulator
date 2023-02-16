#ifndef INC_8_BIT_CPU_EMULATOR_MAIN_H
#define INC_8_BIT_CPU_EMULATOR_MAIN_H

#endif //INC_8_BIT_CPU_EMULATOR_MAIN_H

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

#define MEMORY 65535
#define EXPECTED_PROGRAM_WORDS 256
#define BLOCK_SIZE 4096
#define MAX_INPUT_LENGTH 1024
#define STACK_SIZE 8
#define STACK_SIZE 8
#define TASK_PARALLEL 4

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
#define OP_TSK 0x16
#define OP_SCH 0x17
#define OP_SWT 0x18
#define OP_KIL 0x19

#define TIME_SLOT 15

typedef struct {
    uint8_t data[STACK_SIZE];
    int top;
} ShiftStack;

struct memory_block {
    uint16_t startAddress;
    uint16_t size;
};

typedef struct {
    struct memory_block programMemory;
    struct memory_block usableMemory;
    struct memory_block memoryBlock;
    struct memory_block currentFlashBlock;
} MemoryMap;

typedef struct {
    uint8_t pid; // unique id of the task
    uint8_t priority; // priority of the task
    uint8_t entry_point; // entry point of the task
    uint8_t *program_counter; // pc of task, relative to entry point
    uint8_t status; // status
    uint8_t time_slice; // Time dedicated to the task, depends on priority
    uint8_t time_running; // Time running the task
} Task;

typedef struct {
    Task **tasks; // array of pointers to tasks
    uint8_t size; // number of tasks in the queue
    uint8_t head; // index of the next task to be executed
} TaskQueue;

typedef struct {
    //Memory map
    MemoryMap mm;
    // General-purpose registers + 16 is PC
    uint8_t* reg;

    // Memory
    uint8_t* memory;

    // Stack shift register
    ShiftStack ssr;

    // ALU Flags register
    bool z_flag;
    bool v_flag;

    // Multitask
    bool scheduler;
    bool cpu_mode;
    TaskQueue* task_queue;
} CPUState;

int start(size_t program_size, size_t flash_size, const uint8_t* program_memory, uint8_t** flash_memory, uint8_t* memory);
uint8_t load_program(char *program_file, uint8_t **program_memory);
int load_flash(char *flash_file, FILE *fpf, uint8_t ***flash_memory);

uint8_t count_leading_zeros(uint8_t x);
void push(ShiftStack *stack, uint8_t value);
uint8_t pop( ShiftStack *stack);

bool execute_instruction(CPUState *state);

void increment_pc(CPUState *state, uint8_t opcode);

void add(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode);
void subtract(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode);
void multiply(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode);

void initialize_scheduler(TaskQueue *task_queue, uint8_t *program_counter);
uint8_t create_task(TaskQueue *task_queue, uint8_t entry_point);
void schedule(CPUState *state);
void yield_task(TaskQueue *task_queue, uint8_t pid);
void kill_task(TaskQueue *task_queue, uint8_t pid);

uint8_t memory_access(CPUState *state, uint8_t reg, uint16_t address, uint8_t mode, uint8_t srcDest);