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
#define DATA_MEMORY 256
#define EXPECTED_PROGRAM_WORDS 255
#define EXPECTED_FLASH_WORDS 255
#define MAX_INPUT_LENGTH 1024
#define STACK_SIZE 8

typedef struct {
    uint8_t data[STACK_SIZE];
    int top;
} ShiftStack;


typedef struct {
    // Program counter
    uint8_t pc;

    // General-purpose registers
    uint8_t reg[2];

    // Memory
    uint8_t *program_memory;
    uint8_t *data_memory;
    uint8_t *flash_memory;

    // Stack shift register
    ShiftStack ssr;

    // ALU Flags register
    bool z_flag;
    bool v_flag;
} CPUState;

int start(uint8_t *program_memory, uint8_t *data_memory, uint8_t *flash_memory);
void load_program(char *program_file, uint16_t **program_memory);
void load_flash(char *flash_file, FILE *fpf, uint8_t **flash_memory);

uint8_t count_leading_zeros(uint8_t x);
void push(ShiftStack *stack, uint8_t value);
uint8_t pop( ShiftStack *stack);

bool execute_instruction(CPUState *state);