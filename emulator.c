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
    //state->reg = malloc(16 * sizeof(uint8_t));]
    // debug shit
//    bool goOut = false;
//    while(!goOut) {
//        printf("h");
//        sleep(1);
//    }
    state->pc = calloc(1, sizeof(uint16_t));
    state->v_flag = false;
    state->z_flag = false;
    state->memory = memory;
    printf("From emulator, register pointer: %p\n", state->reg);
    state->inSubroutine = &(state->memory[state->mm.flagsBlock.startAddress]);
    memcpy(state->memory, program_memory, program_size);

    setupMmap(state, program_size);

    if (flash_size > BLOCK_SIZE) {
        memcpy(&(state->memory[state->mm.currentFlashBlock.startAddress]), flash_memory[0], 4096);
    } else {
        memcpy(&(state->memory[state->mm.currentFlashBlock.startAddress]), flash_memory[0], flash_size);
    }

    printf("Starting emulator\n");
    bool exitCode = false;
    while (*(state->pc) + 1 < UINT16_MAX && !exitCode) {
        //sleep(1);
        exitCode = execute_instruction(state);
    }
    if (*(state->pc) + 1 >= UINT16_MAX) printf("PC went over 0xffff\n");
    return 0;
}