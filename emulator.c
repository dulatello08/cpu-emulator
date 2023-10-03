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
    state->enable_mask_interrupts = false;
    state->i_queue->size = 0;
    //printf("From emulator, register pointer: %p\n", (void *) state->reg);
    memcpy(state->memory, program_memory, program_size);

    setupMmap(state, program_size);
    state->in_subroutine = &(state->memory[state->mm.flagsBlock.startAddress]);
    printf("Flash size: %zu\n",  flash_size);
    if (flash_size > BLOCK_SIZE) {
        memcpy(&(state->memory[state->mm.currentFlashBlock.startAddress]), flash_memory[0], 4096);
    } else {
        memcpy(&(state->memory[state->mm.currentFlashBlock.startAddress]), flash_memory[0], flash_size);
    }

    printf("Starting emulator\n");
    bool exitCode = false;
    
    printf("emulator pointer: %p\n", (void *) state->i_queue);
    while (*(state->pc) + 1 < UINT16_MAX && !exitCode) {
        printf("enable mask interrupts %s, queue size: %02x\n", state->enable_mask_interrupts ? "true": "false", state->i_queue->size);
        if (!state->enable_mask_interrupts || state->i_queue->size == 0) {
            exitCode = execute_instruction(state);
        } else {
            uint8_t i_source = pop_interrupt(state->i_queue);
            printf("source: %02x", i_source);
            uint16_t i_handler = get_interrupt_handler(state->i_vector_table, i_source);
            printf("before interrupt pc: %x\n", *state->pc);
            printf("interrupt jump to %x\n", i_handler);
            pushStack(state, *state->pc & 0xFF);
            pushStack(state, (*state->pc >> 8) & 0xFF);
            *(state->pc) = i_handler;
            *(state->in_subroutine) = true;
        }
        usleep(100000);
    }
    if (*(state->pc) + 1 >= UINT16_MAX) printf("PC went over 0xffff\n");
    return 0;
}
