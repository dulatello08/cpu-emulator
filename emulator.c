#include <signal.h>

#include "main.h"
#include <stdio.h>
// ReSharper disable once CppParameterMayBeConstPtrOrRef
int start(AppState *appState) {
    //state->reg = malloc(16 * sizeof(uint8_t));
    // debug stuff
//    bool goOut = false;
//    while(!goOut) {
//        printf("h");
//        sleep(1);
//    }
//    uint8_t interrupt_vector_table;
    appState->state->pc = calloc(1, sizeof(uint16_t));
    appState->state->v_flag = false;
    appState->state->z_flag = false;
    appState->state->memory = appState->shared_data_memory;
    appState->state->enable_mask_interrupts = false;
    //*state->i_queue->size = 0;
    //printf("From emulator, register pointer: %p\n", (void *) state->reg);
    memcpy(appState->state->memory, appState->program_memory, appState->program_size);

    setupMmap(appState->state, appState->program_size);
    appState->state->in_subroutine = &(appState->state->memory[appState->state->mm.flagsBlock.startAddress]);
    printf("Flash size: %d\n", appState->flash_size);
    if (appState->flash_size > BLOCK_SIZE) {
        memcpy(&(appState->state->memory[appState->state->mm.currentFlashBlock.startAddress]), appState->flash_memory[0], 4096);
    } else {
        memcpy(&(appState->state->memory[appState->state->mm.currentFlashBlock.startAddress]), appState->flash_memory[0], appState->flash_size);
    }

    printf("Starting emulator\n");
    bool exitCode = false;

    while (*(appState->state->pc) + 1 < UINT16_MAX && !exitCode) {
        if (appState->gui_shm != NULL) {
            appState->state->memory[appState->state->mm.peripheralControl.startAddress + 3] = appState->gui_shm->keyboard_o[0];
            appState->state->memory[appState->state->mm.peripheralControl.startAddress + 4] = appState->gui_shm->keyboard_o[1];
            if (appState->gui_shm->i_queue.size > 0) {
                memcpy(appState->state->i_queue->sources, appState->gui_shm->i_queue.sources, INTERRUPT_QUEUE_MAX);
                *(appState->state->i_queue->size) = appState->gui_shm->i_queue.size;
            }
            if (appState->state->memory[appState->state->mm.flagsBlock.startAddress + 1]) {
                clear_display(appState->gui_shm->display);
                memcpy(appState->gui_shm->display, appState->state->display, sizeof(appState->state->display));
                print_display(appState->gui_shm->display);
                kill(appState->gui_pid, SIGUSR1);
                appState->state->memory[appState->state->mm.flagsBlock.startAddress + 1] -= 1;
            }
        }
        if (!appState->state->enable_mask_interrupts || *appState->state->i_queue->size == 0) {
            //printf("0x%04x\n", *appState->state->pc);
            //printf("queue size %d\n", *appState->state->i_queue->size);
            exitCode = execute_instruction(appState->state);
        } else {
            const uint8_t i_source = pop_interrupt(appState->state->i_queue);
            if(appState->gui_shm != NULL && i_source == 0x01) {
                memcpy(appState->gui_shm->i_queue.sources, appState->state->i_queue->sources, INTERRUPT_QUEUE_MAX);
                appState->gui_shm->i_queue.size = *(appState->state->i_queue->size);
            }
            printf("source: %02x\n", i_source);
            const uint16_t i_handler = get_interrupt_handler(appState->state->i_vector_table, i_source);
            printf("before interrupt pc: %x\n", *appState->state->pc);
            printf("interrupt jump to %x\n", i_handler);
            pushStack(appState->state, *appState->state->pc & 0xFF);
            pushStack(appState->state, (*appState->state->pc >> 8) & 0xFF);
            *(appState->state->pc) = i_handler;
            *(appState->state->in_subroutine) = true;
        }
        usleep(1000);
    }
    if (*(appState->state->pc) + 1 >= UINT16_MAX) printf("PC went over 0xffff\n");
    free(appState->state->pc);
    return 0;
}
