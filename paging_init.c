//
// Created by Dulat S on 11/4/24.
//
#include "main.h"

void initialize_page_table(CPUState *state, uint8_t *boot_sector_buffer, size_t boot_size) {
    MemoryConfig *mem_config = &state->memory_config;
    PageTable *page_table = create_page_table();
    state->page_table = page_table;

    // Traverse each memory section in the memory map
    for (size_t i = 0; i < mem_config->section_count; ++i) {
        MemorySection *section = &mem_config->sections[i];
        // Initialize based on the section type
        switch (section->type) {
            case BOOT_SECTOR:
                // Copy the boot sector to the allocated pages
                bulk_copy_memory(state, section->start_address, (const uint16_t *)boot_sector_buffer, boot_size / sizeof(uint16_t));
                break;

            case USABLE_MEMORY:
            case MMIO_PAGE:
            case FLASH:
            case UNKNOWN_TYPE:
            default:
                break;
        }
    }
}
