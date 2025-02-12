#include <signal.h>

#include "main.h"
#include <stdio.h>
// ReSharper disable once CppParameterMayBeConstPtrOrRef
int start(AppState *appState) {
    // debug stuff
    appState->state->pc = calloc(1, sizeof(uint32_t));
    appState->state->v_flag = false;
    appState->state->z_flag = false;
    appState->state->enable_mask_interrupts = false;

    printf("Starting emulator\n");
    bool exitCode = false;
    MemoryConfig *mc = &appState->state->memory_config;
    printf("Memory Config: %zu sections\n", mc->section_count);

    for (size_t i = 0; i < mc->section_count; i++) {
        MemorySection *sec = &mc->sections[i];
        printf("  [%zu] %s - Type: %d, Start: 0x%X, Pages: %u",
               i,
               sec->section_name,
               sec->type,
               sec->start_address,
               sec->page_count);
        if (sec->device[0])  // Only print device info if non-empty
            printf(", Device: %s", sec->device);
        printf("\n");
    }

    while (*(appState->state->pc) + 1 < UINT32_MAX && !exitCode) {
        if (appState->gui_shm != NULL) {
//            appState->state->memory[appState->state->mm.peripheralControl.startAddress + 3] = appState->gui_shm->keyboard_o[0];
//            appState->state->memory[appState->state->mm.peripheralControl.startAddress + 4] = appState->gui_shm->keyboard_o[1];
//            if (appState->gui_shm->i_queue.size > 0) {
//                memcpy(appState->state->i_queue->sources, appState->gui_shm->i_queue.sources, INTERRUPT_QUEUE_MAX);
//                *(appState->state->i_queue->size) = appState->gui_shm->i_queue.size;
//            }
//            if (appState->state->memory[appState->state->mm.flagsBlock.startAddress + 1]) {
//                clear_display(appState->gui_shm->display);
//                memcpy(appState->gui_shm->display, appState->state->display, sizeof(appState->state->display));
//                print_display(appState->gui_shm->display);
//                appState->state->memory[appState->state->mm.flagsBlock.startAddress + 1] -= 1;
//            }
        }
        if (!appState->state->enable_mask_interrupts || *appState->state->i_queue->size == 0) {
//            printf("0x%04x\n", *appState->state->pc);
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
        }
        // usleep(10000);
    }
    if (*(appState->state->pc) + 1 >= UINT16_MAX) printf("PC went over 0xffff\n");
    free(appState->state->pc);
    return 0;
}
