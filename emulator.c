#include <signal.h>

#include "main.h"
#include <stdio.h>
#include "uart.h"
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
        // Check if the interrupt queue is not empty.
        if (!is_interrupt_queue_empty(appState->state->i_queue)) {
            uint8_t irq;
            dequeue_interrupt(appState->state->i_queue, &irq);
            if (irq==0) {
                uint8_t value;
                uart_read(appState->state->uart, &value);
                write8(appState->state, 0x10001, value);
                printf("%02x\n", value);
            }
            InterruptVectorEntry *ive = get_interrupt_vector(appState->state->i_vector_table, irq);
            if (ive != NULL) {
                // Save the current PC as the return address.
                uint32_t return_address = *(appState->state->pc);
                // Push the return address onto the stack (as a 32-bit value split into 4 bytes).
                pushStack(appState->state, (uint8_t)(return_address & 0xFF));
                pushStack(appState->state, (uint8_t)((return_address >> 8) & 0xFF));
                pushStack(appState->state, (uint8_t)((return_address >> 16) & 0xFF));
                pushStack(appState->state, (uint8_t)((return_address >> 24) & 0xFF));

                printf("Interrupt %d received: pushing return address 0x%08x and jumping to ISR at 0x%08x\n",
                       irq, return_address, ive->handler_address);

                // Set the program counter to the ISR handler address.
                *(appState->state->pc) = ive->handler_address;
            } else {
                printf("Interrupt %d received but no ISR registered.\n", irq);
            }
        }

        // Execute the next instruction.
        exitCode = execute_instruction(appState->state);
        usleep(1000);
    }
    return 0;
}
