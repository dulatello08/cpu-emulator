//
// Example Byte-Only Memory Code
//
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------
#define PAGE_SHIFT 12               // Log2 of PAGE_SIZE
#define PAGE_SIZE  (1 << PAGE_SHIFT)

// -----------------------------------------------------------------------------
// Data Structures
// -----------------------------------------------------------------------------
typedef struct PageTableEntry {
    struct PageTableEntry* next;
    struct PageTableEntry* prev;
    bool     is_allocated;
    uint32_t page_index;
    uint8_t* page_data; // Byte array for this page
} PageTableEntry;

typedef struct PageTable {
    PageTableEntry* head;
    PageTableEntry* tail;
    uint32_t        page_count;
} PageTable;

typedef struct CPUState {
    PageTable* page_table;
    // other CPU fields as needed
} CPUState;

// -----------------------------------------------------------------------------
// Page Table Management
// -----------------------------------------------------------------------------
PageTable* create_page_table(void) {
    PageTable* table = (PageTable*)malloc(sizeof(PageTable));
    if (!table) {
        fprintf(stderr, "Failed to allocate memory for PageTable.\n");
        exit(EXIT_FAILURE);
    }
    table->head       = NULL;
    table->tail       = NULL;
    table->page_count = 0;
    return table;
}

PageTableEntry* allocate_page(PageTable* table, uint32_t page_index) {
    PageTableEntry* new_page = (PageTableEntry*)malloc(sizeof(PageTableEntry));
    if (!new_page) {
        fprintf(stderr, "Memory allocation failed for PageTableEntry.\n");
        exit(EXIT_FAILURE);
    }

    new_page->page_data = (uint8_t*)calloc(PAGE_SIZE, sizeof(uint8_t));
    if (!new_page->page_data) {
        fprintf(stderr, "Memory allocation failed for page data.\n");
        free(new_page);
        exit(EXIT_FAILURE);
    }

    new_page->is_allocated = true;
    new_page->page_index   = page_index;
    new_page->next         = NULL;
    new_page->prev         = table->tail;

    if (table->tail) {
        table->tail->next = new_page;
    } else {
        table->head = new_page;
    }
    table->tail = new_page;
    table->page_count++;

    return new_page;
}

static inline PageTableEntry* find_or_allocate_page(PageTable* table,
                                                    uint32_t page_index,
                                                    bool allocate_if_unallocated)
{
    // If table is empty, allocate immediately if requested
    if (!table->head) {
        if (allocate_if_unallocated) {
            return allocate_page(table, page_index);
        }
        return NULL;
    }

    // Decide whether to search from head or tail
    PageTableEntry* current;
    if ((page_index - table->head->page_index) < (table->tail->page_index - page_index)) {
        // Start from head
        current = table->head;
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

    // If page is found
    if (current && current->page_index == page_index) {
        return current;
    }

    // Not found, allocate if needed
    if (allocate_if_unallocated) {
        PageTableEntry* new_page = allocate_page(table, page_index);

        // Insert new_page in the correct position
        if (!current) {
            // Insert at the end (already appended by allocate_page)
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

    // Page not found, and we aren't allocating
    return NULL;
}

// -----------------------------------------------------------------------------
// 8-Bit (Byte) Access
// -----------------------------------------------------------------------------
/**
 * Returns a pointer to the single byte at 'address'.
 * If 'allocate_if_unallocated' is true, the page is allocated if needed.
 * Returns NULL on error/invalid access.
 */
uint8_t* get_memory_ptr(CPUState* state, uint32_t address, bool allocate_if_unallocated)
{
    uint32_t page_index = address >> PAGE_SHIFT;           // which page
    uint32_t offset     = address & (PAGE_SIZE - 1);       // byte offset

    PageTableEntry* page = find_or_allocate_page(state->page_table, page_index, allocate_if_unallocated);
    if (!page || !page->is_allocated) {
        fprintf(stderr, "Memory access violation at address 0x%08x\n", address);
        return NULL;
    }
    return page->page_data + offset;
}

/**
 * Read a single byte (8 bits) from memory.
 * Returns 0 on error/invalid.
 */
uint8_t get_memory(CPUState* state, uint32_t address) {
    uint8_t* mem_ptr = get_memory_ptr(state, address, false);
    if (!mem_ptr) {
        // handle error
        return 0;
    }
    return *mem_ptr;
}

/**
 * Write a single byte (8 bits) to memory,
 * allocating a new page if needed.
 */
void set_memory(CPUState* state, uint32_t address, uint8_t value) {
    uint8_t* mem_ptr = get_memory_ptr(state, address, true);
    if (!mem_ptr) {
        // handle error
        return;
    }
    *mem_ptr = value;
}

// static void free_page(PageTable* table, PageTableEntry* page) {
//     if (!page || !page->is_allocated) return;
//
//     free(page->page_data);
//     page->page_data = NULL;
//     page->is_allocated = false;
//
//     if (page->prev) {
//         page->prev->next = page->next;
//     } else {
//         table->head = page->next;
//     }
//     if (page->next) {
//         page->next->prev = page->prev;
//     } else {
//         table->tail = page->prev;
//     }
//     table->page_count--;
//     free(page);
// }

void free_all_pages(PageTable* table) {
    PageTableEntry* current = table->head;
    while (current) {
        PageTableEntry* next = current->next;
        free(current->page_data);
        free(current);
        current = next;
    }
    free(table);
}
