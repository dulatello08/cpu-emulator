//
// Created by Dulat S on 11/4/24.
//
#include "main.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * Compute total bytes in a MemorySection based on its page count
 */
static inline size_t get_section_size_in_bytes(const MemorySection *section) {
    return section->page_count * PAGE_SIZE;  // purely bytes
}

/**
 * Initialize the page table and load the boot sector into memory (in bytes).
 * All 16-bit logic is removed; everything is handled as 8-bit.
 */
void initialize_page_table(CPUState *state, uint8_t *boot_sector_buffer, size_t boot_size) {
    if (state->page_table) {
        free_all_pages(state->page_table);
    }

    MemoryConfig *mem_config = &state->memory_config;

    // Create the page table
    PageTable *page_table = create_page_table();
    state->page_table = page_table;

    // Go through each memory section
    for (size_t i = 0; i < mem_config->section_count; ++i) {
        MemorySection *section = &mem_config->sections[i];

        switch (section->type) {
            case BOOT_SECTOR: {
                // Calculate how many bytes this BOOT_SECTOR can hold
                size_t section_size_bytes = get_section_size_in_bytes(section);

                // If the boot file is larger than this section, error out
                if (boot_size > section_size_bytes) {
                    fprintf(stderr,
                            "[ERROR] Boot file size (%zu bytes) exceeds boot sector size (%zu bytes)\n",
                            boot_size, section_size_bytes);
                    return; // or handle as needed
                }

                // Copy the entire boot sector in a single byte-based operation
                bulk_copy_memory(
                    state,
                    section->start_address,              // destination (in CPU memory)
                    boot_sector_buffer,                  // source (host buffer)
                    boot_size                            // number of BYTES to copy
                );

                // If boot file is smaller than the section, pad the remainder with zeros
                size_t remainder_bytes = section_size_bytes - boot_size;
                if (remainder_bytes > 0) {
                    // Allocate a temporary zero-filled buffer
                    uint8_t *zero_buffer = (uint8_t *)calloc(remainder_bytes, sizeof(uint8_t));
                    if (!zero_buffer) {
                        fprintf(stderr, "[ERROR] Unable to allocate zero buffer for padding.\n");
                        return;
                    }

                    // Copy zeros to the remainder of the BOOT_SECTOR
                    bulk_copy_memory(
                        state,
                        section->start_address + (uint32_t)boot_size,
                        zero_buffer,
                        remainder_bytes
                    );

                    // Clean up
                    free(zero_buffer);
                }
                break;
            }

            case STACK: {
                // For each page in the STACK section, call get_memory_ptr on one byte
                for (unsigned int page = 0; page < section->page_count; ++page) {
                    // Calculate the start address for the current page
                    unsigned int address = section->start_address + (page * PAGE_SIZE);
                    // Touch one byte in the page to allocate it.
                    get_memory_ptr(state, address, true);
                }
                break;
            }
            // You can add similar logic for these if needed
            case USABLE_MEMORY:
            case MMIO_PAGE:
            case FLASH:
            case UNKNOWN_TYPE:
            default:
                // No special handling in this example
                break;
        }
    }
}
