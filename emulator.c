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
        exitCode = execute_instruction(appState->state);
        // usleep(100000);
    }
    if (*(appState->state->pc) + 1 >= UINT16_MAX) printf("PC went over 0xffff\n");
    free(appState->state->pc);
    return 0;
}
