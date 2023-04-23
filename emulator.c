#include "main.h"
#include <stdint.h>

uint8_t count_leading_zeros(uint8_t x) {
    uint8_t count = 0;

    while (x != 0) {
        x >>= 1;
        count++;
    }

    return 8 - count;
}

int start(CPUState *state, size_t program_size, size_t flash_size, const uint8_t* program_memory, uint8_t** flash_memory, uint8_t* memory) {
    state->reg = malloc(16 * sizeof(uint8_t));
    state->pc = calloc(1, sizeof(uint16_t));
    state->v_flag = false;
    state->z_flag = false;
    state->scheduler = false;
    state->memory = memory;
    state->inSubroutine = &(state->memory[state->mm.flagsBlock.startAddress]);
    memcpy(state->memory, program_memory, program_size);

    setupMmap(state, program_size);

    if (flash_size > BLOCK_SIZE) {
        memcpy(&(state->memory[state->mm.currentFlashBlock.startAddress]), flash_memory[0], 4096);
    } else {
        memcpy(&(state->memory[state->mm.currentFlashBlock.startAddress]), flash_memory[0], flash_size - 1);
    }

    state->task_queue = calloc(1, sizeof(TaskQueue));
    state->task_queue->tasks = calloc(1, sizeof(Task*));
    state->task_queue->tasks[0] = calloc(TASK_PARALLEL, sizeof(Task));
    printf("Starting emulator\n");
    bool exitCode = false;
    while (*(state->pc) + 1 < UINT16_MAX && !exitCode) {
        if (!state->scheduler) {
            exitCode = execute_instruction(state);
        } else {
            printf("Entering scheduling loop\n");
            break;
        }
    }
    if (state->scheduler) {
       schedule(state);
    }
    return 0;
}