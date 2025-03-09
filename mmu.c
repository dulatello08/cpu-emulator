//
// Created by dulat on 2/17/23.
//
// Prevents compilation errors with a dummy function or variable.
#include "main.h"
#include <arpa/inet.h>

void memory_write_trigger(CPUState *state, uint32_t address, uint32_t value) {
    MemoryConfig *config = &state->memory_config;
    size_t lo = 0;
    size_t hi = config->section_count;
    MemorySection *section = NULL;

    // Binary search to find the section with the highest start_address <= address.
    // We use [lo, hi) as our search interval.
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (config->sections[mid].start_address <= address) {
            section = &config->sections[mid];
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }

    // Check if we found a section and if the address is within its bounds.
    if (section != NULL) {
        // uint32_t sec_start = section->start_address;
        uint32_t sec_end = section->start_address + section->page_count * PAGE_SIZE;
        if (address < sec_end) {
            if ((section->type == BOOT_SECTOR) || (section->type == USABLE_MEMORY)) {
                return; // normal write – no trigger
            } else if (section->type == MMIO_PAGE) {
                // Handle writes that trigger an action
                if (strcmp(section->device, "UART") == 0) {
                    if (address == 0x10000) {
                        printf("%c", (uint8_t) value);
                        fflush(stdout);
                    }
                }
                if (strcmp(section->device, "PIC") == 0) {
                    if (address == 0x20004) {
                        // Read the base address of the IVT and its length
                        uint32_t ivt_base = read32(state, 0x20000);
                        uint8_t ivt_length = (uint8_t) value;
                        printf("Loading IVT at address %08x, length %02x\n", ivt_base, ivt_length);

                        // Cast the base address to a pointer to uint32_t since each entry is 32-bit
                        uint32_t *ivt_entries = (uint32_t*) get_memory_ptr(state, ivt_base, false);

                        // Populate the IVT using the provided function
                        for (uint8_t source = 0; source < ivt_length; source++) {
                            uint32_t handler_address = ivt_entries[source];
                            handler_address = ntohl(handler_address);
                            if (register_interrupt_vector(state->i_vector_table, source, handler_address)) {
                                printf("Registered interrupt vector for source %u: handler=0x%08x\n", source, handler_address);
                            } else {
                                printf("Error: Failed to register interrupt vector for source %u\n", source);
                            }
                        }
                    }
                }
            }
        }
    }
}
