#include "main.h"
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
    
    printf("emulator pointer: %p\n", (void *) appState->state->i_queue);
    while (*(appState->state->pc) + 1 < UINT16_MAX && !exitCode) {
        if (appState->state->memory[appState->state->mm.flagsBlock.startAddress + 1]) {
            FILE* gui_stdin = fdopen(appState->gui_pipes.stdin_fd, "w");
            for (int j = 0; j < LCD_HEIGHT; j++) {
                for (int i = 0; i < LCD_WIDTH; i++) {
                    char character = appState->state->display[i][j];
                    // Replace null character with '*'
                    if (character == '\0') {
                        character = '*';
                    }
                    fprintf(gui_stdin, "%c", character);
                }
                fprintf(gui_stdin, "\n");
            }
        }
        if (!appState->state->enable_mask_interrupts || *appState->state->i_queue->size == 0) {
            exitCode = execute_instruction(appState->state);
        } else {
            const uint8_t i_source = pop_interrupt(appState->state->i_queue);
            printf("source: %02x", i_source);
            const uint16_t i_handler = get_interrupt_handler(appState->state->i_vector_table, i_source);
            printf("before interrupt pc: %x\n", *appState->state->pc);
            printf("interrupt jump to %x\n", i_handler);
            pushStack(appState->state, *appState->state->pc & 0xFF);
            pushStack(appState->state, (*appState->state->pc >> 8) & 0xFF);
            *(appState->state->pc) = i_handler;
            *(appState->state->in_subroutine) = true;
        }
        // usleep(100000);
    }
    if (*(appState->state->pc) + 1 >= UINT16_MAX) printf("PC went over 0xffff\n");
    return 0;
}
