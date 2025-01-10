//
// Created by Dulat S on 11/4/24.
//
#include "main.h"

static inline size_t get_section_size_in_bytes(const MemorySection *section) {
    return section->page_count * PAGE_SIZE;
}

void initialize_page_table(CPUState *state, uint8_t *boot_sector_buffer, size_t boot_size) {
    MemoryConfig *mem_config = &state->memory_config;
    PageTable *page_table = create_page_table();
    state->page_table = page_table;

    // Traverse each memory section in the memory map
    for (size_t i = 0; i < mem_config->section_count; ++i) {
        MemorySection *section = &mem_config->sections[i];
        // Initialize based on the section type
        switch (section->type) {
            case BOOT_SECTOR: {
                // Calculate how many bytes are available for this BOOT_SECTOR section
                size_t section_size_bytes = get_section_size_in_bytes(section);

                // If the boot file is bigger than the section, error out
                if (boot_size > section_size_bytes) {
                    fprintf(stderr,
                            "[ERROR] Boot file size (%zu bytes) exceeds boot sector section size (%zu bytes)\n",
                            boot_size, section_size_bytes);
                    // Handle error as appropriate (return, exit, etc.)
                    return;
                }

                // Copy as many 16-bit words as fit from boot_sector_buffer to memory
                size_t word_count_to_copy = boot_size / sizeof(uint16_t);
                bulk_copy_memory(state,
                                 section->start_address,
                                 (const uint16_t *)boot_sector_buffer,
                                 word_count_to_copy);
                size_t bytes_copied = word_count_to_copy * sizeof(uint16_t);
                size_t remaining_bytes = boot_size - bytes_copied;
                if (remaining_bytes > 0) {
                    // Calculate the destination address for the last byte
                    uint32_t last_byte_address = section->start_address + (uint32_t)bytes_copied;

                    // Get a pointer to the memory location for the remaining byte
                    uint8_t *dest_ptr = (uint8_t *)get_memory_ptr(state, last_byte_address, true);
                    if (!dest_ptr) {
                        fprintf(stderr, "Failed to get memory pointer at address 0x%08x\n", last_byte_address);
                        return;
                    }

                    // Copy the single remaining byte
                    *dest_ptr = boot_sector_buffer[bytes_copied];
                }

                // If the boot file is smaller than the section, pad the remainder with zeros
                size_t remainder_bytes = section_size_bytes - boot_size;
                if (remainder_bytes > 0) {
                    // Calculate the number of 16-bit words to fill with zeros
                    size_t remainder_word_count = remainder_bytes / sizeof(uint16_t);

                    // Allocate and initialize a zero-filled buffer for padding
                    uint16_t *zero_buffer = calloc(remainder_word_count, sizeof(uint16_t));
                    if (!zero_buffer) {
                        fprintf(stderr, "[ERROR] Unable to allocate zero buffer for padding.\n");
                        return;
                    }

                    // Copy zeros to the remainder of the boot sector section
                    bulk_copy_memory(state,
                                     section->start_address + boot_size,
                                     zero_buffer,
                                     remainder_word_count);

                    // Clean up the temporary buffer
                    free(zero_buffer);
                }
                break;
            }
            case USABLE_MEMORY:
            case MMIO_PAGE:
            case FLASH:
            case UNKNOWN_TYPE:
            default:
                break;
        }
    }
}
