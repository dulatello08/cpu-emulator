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

#define DATA_MEMORY 256
#define EXPECTED_PROGRAM_WORDS 255
#define EXPECTED_FLASH_WORDS 255
#define MAX_INPUT_LENGTH 1024
#define STACK_SIZE 8
#define TASK_PARALLEL 4

#define TIME_SLOT 15

typedef struct {
    uint8_t data[STACK_SIZE];
    int top;
} ShiftStack;

typedef struct {
    uint8_t pid; // unique id of the task
    uint8_t priority; // priority of the task
    uint8_t *program_counter; // pointer to the task's program counter
    uint16_t *program_memory; // program of task
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
    // Program counter
    uint8_t pc;

    // General-purpose registers
    uint8_t reg[2];

    // Memory
    uint16_t *program_memory;
    uint8_t *data_memory;

    // Stack shift register
    ShiftStack ssr;

    // ALU Flags register
    bool z_flag;
    bool v_flag;

    // Multitask
    bool scheduler;
    bool cpu_mode;
    TaskQueue *task_queue;
} CPUState;

int start(const uint16_t *program_memory, uint8_t *data_memory, uint8_t *flash_memory);
void load_program(char *program_file, uint16_t **program_memory);
void load_flash(char *flash_file, FILE *fpf, uint8_t **flash_memory);
bool execute_instruction(CPUState *state, uint8_t *flash_memory);

uint8_t pop(ShiftStack *stack);
void push(ShiftStack *stack, uint8_t value);
uint8_t count_leading_zeros(uint8_t x);
void create_uint16_array(const uint8_t *original_array, uint16_t *new_array, int start_index, int size);

bool execute_sch_instruction(CPUState *state, uint8_t *program_counter, const uint16_t *program_memory, uint8_t *flash_memory);
void initialize_scheduler(TaskQueue *task_queue, uint8_t *program_counter);
uint8_t create_task(TaskQueue *task_queue, uint8_t entry_point);
void schedule(TaskQueue *task_queue);
void yield_task(TaskQueue *task_queue, uint8_t pid);
void kill_task(TaskQueue *task_queue, uint8_t pid);