//
// Created by Dulat S on 10/30/24.
//

#include "main.h"

#define PAGE_SHIFT 12               // Log2 of PAGE_SIZE

// Function to create and initialize a new page table
PageTable* create_page_table(void) {
    PageTable* table = (PageTable*)malloc(sizeof(PageTable));
    if (!table) {
        fprintf(stderr, "Failed to allocate memory for PageTable.\n");
        exit(EXIT_FAILURE);
    }
    table->head = table->tail = NULL;
    table->page_count = 0;
    return table;
}

// Optimized helper function to allocate and add a new page to the page table
PageTableEntry* allocate_page(PageTable *table, uint32_t page_index) {
    PageTableEntry* new_page = (PageTableEntry*)malloc(sizeof(PageTableEntry));
    if (!new_page) {
        fprintf(stderr, "Memory allocation failed for PageTableEntry.\n");
        exit(EXIT_FAILURE);
    }

    new_page->page_data = (uint8_t *)calloc(PAGE_SIZE, sizeof(uint8_t));
    if (!new_page->page_data) {
        fprintf(stderr, "Memory allocation failed for page data.\n");
        free(new_page);
        exit(EXIT_FAILURE);
    }

    new_page->is_allocated = true;
    new_page->page_index = page_index;
    new_page->next = NULL;
    new_page->prev = table->tail;

    // Append the new page to the end of the list
    if (table->tail) {
        table->tail->next = new_page;
    } else {
        table->head = new_page; // First page being added
    }
    table->tail = new_page;
    table->page_count++;

    return new_page;
}

// Optimized helper function to find a page by its index, allocating if necessary
static inline PageTableEntry* find_or_allocate_page(PageTable *table, uint32_t page_index, bool allocate_if_unallocated) {
    // Optimization: Use a skip list approach for faster traversal
    PageTableEntry *current = table->head;

    // Early exit if table is empty
    if (!current) {
        if (allocate_if_unallocated) {
            return allocate_page(table, page_index);
        } else {
            return NULL;
        }
    }

    // Optimization: Traverse from head or tail depending on page_index
    if (page_index - table->head->page_index < table->tail->page_index - page_index) {
        // Start from head
        while (current && current->page_index < page_index) {
            current = current->next;
        }
    } else {
        // Start from tail
        current = table->tail;
        while (current && current->page_index > page_index) {
            current = current->prev;
        }
    }

    // If the page is found
    if (current && current->page_index == page_index) {
        return current;
    }

    if (allocate_if_unallocated) {
        // Allocate a new page
        PageTableEntry* new_page = allocate_page(table, page_index);

        // Insert the new page in the correct position
        if (!current) {
            // Insert at the end
            // Already handled in allocate_page
        } else if (current->page_index > page_index) {
            // Insert before current
            new_page->next = current;
            new_page->prev = current->prev;
            if (current->prev) {
                current->prev->next = new_page;
            } else {
                table->head = new_page;
            }
            current->prev = new_page;
        } else {
            // Insert after current
            new_page->prev = current;
            new_page->next = current->next;
            if (current->next) {
                current->next->prev = new_page;
            } else {
                table->tail = new_page;
            }
            current->next = new_page;
        }
        return new_page;
    }

    return NULL;  // Page not found and allocation was not requested
}

/**
 * Optimized helper function to get a pointer to the memory location.
 * Returns NULL if the address is invalid.
 */
uint16_t* get_memory_ptr(CPUState *state, uint32_t address, bool allocate_if_unallocated) {
    uint32_t page_index = address >> PAGE_SHIFT;
    uint32_t offset = address & (PAGE_SIZE - 1);

    // Find or allocate the page in the page table
    PageTableEntry *page = find_or_allocate_page(state->page_table, page_index, allocate_if_unallocated);

    // Return NULL if the page is unallocated after the allocation attempt
    if (!page || !page->is_allocated) {
        fprintf(stderr, "Memory access violation at address 0x%08x\n", address);
        return NULL;
    }

    // Return the pointer to the memory location
    return (uint16_t *)(page->page_data + offset);
}

/**
 * Optimized function to get a 16-bit value from the specified address.
 */
uint16_t get_memory(CPUState *state, uint32_t address) {
    uint16_t *mem_ptr = get_memory_ptr(state, address, false);

    // Handle invalid memory access
    if (!mem_ptr) {
        // Depending on requirements, could set an error code or handle exception
        return 0; // Return 0 or handle error appropriately
    }

    // Read and return the value
    return *mem_ptr;
}

/**
 * Optimized function to set a 16-bit value at the specified address, allocating the page if necessary.
 */
void set_memory(CPUState *state, uint32_t address, uint16_t value) {
    uint16_t *mem_ptr = get_memory_ptr(state, address, true); // Request allocation if needed

    // Handle invalid memory access
    if (!mem_ptr) {
        // Depending on requirements, could set an error code or handle exception
        return; // Early exit on error
    }

    // Write the value to memory
    *mem_ptr = value;
}

// Optimized function to free a specific page and remove it from the page table
//static inline void free_page(PageTable* table, PageTableEntry* page) {
//    if (!page || !page->is_allocated) return;
//
//    // Free the page data
//    free(page->page_data);
//    page->page_data = NULL;
//    page->is_allocated = false;
//
//    // Remove the page from the linked list
//    if (page->prev) {
//        page->prev->next = page->next;
//    } else {
//        table->head = page->next; // Page is the head
//    }
//
//    if (page->next) {
//        page->next->prev = page->prev;
//    } else {
//        table->tail = page->prev; // Page is the tail
//    }
//
//    table->page_count--;
//    free(page);
//}

/**
 * Optimized function to free all pages in the page table
 */
//static inline void free_all_pages(PageTable *table) {
//    PageTableEntry *current = table->head;
//    while (current) {
//        PageTableEntry *next = current->next;
//        free(current->page_data);
//        free(current);
//        current = next;
//    }
//    free(table);
//}
