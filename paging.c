//
// Example Byte-Only Memory Code
//
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "main.h"

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------
#define PAGE_SHIFT 12               // Log2 of PAGE_SIZE

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

// -----------------------------------------------------------------------------
// Helper: Allocate a new page without linking it into the list
// -----------------------------------------------------------------------------
static inline PageTableEntry* allocate_new_page(uint32_t page_index) {
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
    new_page->prev         = NULL;
    return new_page;
}

// -----------------------------------------------------------------------------
// Refactored: Find or Allocate Page with Correct Sorted Insertion
// -----------------------------------------------------------------------------
static inline PageTableEntry* find_or_allocate_page(PageTable* table,
                                                    uint32_t page_index,
                                                    bool allocate_if_unallocated)
{
    // If the table is empty, allocate a new page and set as head and tail.
    if (!table->head) {
        if (allocate_if_unallocated) {
            PageTableEntry* new_page = allocate_new_page(page_index);
            table->head = table->tail = new_page;
            table->page_count = 1;
            return new_page;
        }
        return NULL;
    }

    // Decide search direction and record it.
    bool search_from_head = ((page_index - table->head->page_index) <
                             (table->tail->page_index - page_index));

    if (search_from_head) {
        // Search from head: find first node with page_index >= target.
        PageTableEntry* current = table->head;
        while (current && current->page_index < page_index) {
            current = current->next;
        }
        if (current && current->page_index == page_index) {
            return current;  // Found the page.
        }
        if (!allocate_if_unallocated) return NULL;

        // Allocate new page.
        PageTableEntry* new_page = allocate_new_page(page_index);
        // Determine insertion point.
        if (current == table->head) {
            // Insert at the beginning.
            new_page->next = table->head;
            table->head->prev = new_page;
            table->head = new_page;
        } else if (current == NULL) {
            // Target is greater than all nodes; insert at tail.
            new_page->prev = table->tail;
            table->tail->next = new_page;
            table->tail = new_page;
        } else {
            // Insert before the 'current' node.
            new_page->next = current;
            new_page->prev = current->prev;
            if (current->prev) {
                current->prev->next = new_page;
            }
            current->prev = new_page;
        }
        table->page_count++;
        return new_page;

    } else {
        // Search from tail: find the last node with page_index <= target.
        PageTableEntry* current = table->tail;
        while (current && current->page_index > page_index) {
            current = current->prev;
        }
        if (current && current->page_index == page_index) {
            return current;  // Found the page.
        }
        if (!allocate_if_unallocated) return NULL;

        // Allocate new page.
        PageTableEntry* new_page = allocate_new_page(page_index);
        // Determine insertion point.
        if (current == table->tail) {
            // Insert at the end.
            new_page->prev = table->tail;
            table->tail->next = new_page;
            table->tail = new_page;
        } else if (current == NULL) {
            // Target is less than all nodes; insert at head.
            new_page->next = table->head;
            table->head->prev = new_page;
            table->head = new_page;
        } else {
            // Insert after 'current'.
            new_page->next = current->next;
            new_page->prev = current;
            if (current->next) {
                current->next->prev = new_page;
            }
            current->next = new_page;
        }
        table->page_count++;
        return new_page;
    }
}

bool has_cycle(PageTable *table) {
    if (!table || !table->head) {
        return false;
    }
    PageTableEntry *slow = table->head;
    PageTableEntry *fast = table->head;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) {
            return true;
        }
    }
    return false;
}
bool validate_list(PageTable *table) {
    if (!table) {
        fprintf(stderr, "PageTable is NULL\n");
        return false;
    }

    // Check for cycle first.
    if (has_cycle(table)) {
        fprintf(stderr, "Cycle detected in the page table!\n");
        return false;
    }

    // Check next/prev consistency and count nodes.
    size_t count = 0;
    PageTableEntry *current = table->head;
    PageTableEntry *prev = NULL;
    while (current) {
        // Verify that the current node's previous pointer is as expected.
        if (current->prev != prev) {
            fprintf(stderr, "List corruption: inconsistent prev pointer at page index %u\n", current->page_index);
            return false;
        }
        prev = current;
        current = current->next;
        count++;
    }

    if (count != table->page_count) {
        fprintf(stderr, "Page count mismatch: expected %zu, found %zu\n", table->page_count, count);
        return false;
    }
    return true;
}
void dump_page_table(PageTable *table) {
    PageTableEntry *current = table->head;
    while (current) {
        printf("Node %p: page_index=%u, prev=%p, next=%p\n",
               (void*)current, current->page_index,
               (void*)current->prev, (void*)current->next);
        current = current->next;
    }
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
    // Use table->page_count to control the loop.
    for (size_t i = 0; i < table->page_count && current != NULL; i++) {
        if (current->is_allocated && current->page_data) {
            free(current->page_data);
        }
        PageTableEntry* temp = current;
        current = current->next;
        free(temp);
    }
    free(table);
}

