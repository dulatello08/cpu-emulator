#include "main.h"
#include <stdint.h>

int start(CPUState *state, size_t program_size, size_t flash_size, const uint8_t* program_memory, uint8_t** flash_memory, uint8_t* memory) {
    //state->reg = malloc(16 * sizeof(uint8_t));
    // debug stuff
//    bool goOut = false;
//    while(!goOut) {
//        printf("h");
//        sleep(1);
//    }
//    uint8_t interrupt_vector_table;
    state->pc = calloc(1, sizeof(uint16_t));
    state->v_flag = false;
    state->z_flag = false;
    state->memory = memory;

    state->i_queue.sources = NULL;
    state->i_queue.top = SENTINEL_VALUE;
    //printf("From emulator, register pointer: %p\n", (void *) state->reg);
    memcpy(state->memory, program_memory, program_size);

    setupMmap(state, program_size);
    state->in_subroutine = &(state->memory[state->mm.flagsBlock.startAddress]);
    printf("Flash size: %zu",  flash_size);
    if (flash_size > BLOCK_SIZE) {
        memcpy(&(state->memory[state->mm.currentFlashBlock.startAddress]), flash_memory[0], 4096);
    } else {
        memcpy(&(state->memory[state->mm.currentFlashBlock.startAddress]), flash_memory[0], flash_size);
    }

    printf("Starting emulator\n");
    bool exitCode = false;
    while (*(state->pc) + 1 < UINT16_MAX && !exitCode) {
        if (state->i_queue.top == SENTINEL_VALUE || state->i_queue.top == 0)
        //usleep(500000);
        exitCode = execute_instruction(state);
    }
    if (*(state->pc) + 1 >= UINT16_MAX) printf("PC went over 0xffff\n");
    return 0;
}
