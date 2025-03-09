//
// common.h
// Created by Dulat S on 3/8/25.
//

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <sys/types.h>
#include <ctype.h>
#include <pthread.h>
#include "constants.h"  // Ensure constants (like MAX_SECTIONS, MAX_INTERRUPTS, IRQ_QUEUE_SIZE, LCD_WIDTH, LCD_HEIGHT) are defined here

// ----------------------------
// Page Table Definitions
// ----------------------------
typedef struct PageTableEntry {
    uint8_t* page_data;     // Points to physical memory data for this page
    bool is_allocated;      // Indicates if the page is allocated
    uint32_t page_index;    // The logical index of this page (for address calculation)
    struct PageTableEntry* next;  // Pointer to the next page in the list
    struct PageTableEntry* prev;  // Pointer to the previous page in the list
} PageTableEntry;

typedef struct {
    PageTableEntry* head;   // Head of the doubly linked list
    PageTableEntry* tail;   // Tail of the doubly linked list
    size_t page_count;      // Total number of pages in the table
} PageTable;

typedef enum {
    BOOT_SECTOR,
    USABLE_MEMORY,
    MMIO_PAGE,
    FLASH,
    STACK,
    UNKNOWN_TYPE
} PageType;

typedef struct {
    char section_name[64];
    PageType type;
    unsigned int start_address;
    unsigned int page_count;
    char device[64];        // Optional device information for MMIO pages
} MemorySection;

typedef struct {
    MemorySection sections[MAX_SECTIONS];
    size_t section_count;
} MemoryConfig;

// ----------------------------
// Interrupt Definitions
// ----------------------------
typedef struct {
    uint8_t source;
    uint32_t handler_address;   // Address of the interrupt service routine (ISR)
} InterruptVectorEntry;

typedef struct InterruptVectorTable {
    InterruptVectorEntry entries[MAX_INTERRUPTS];  // Array of vector entries
    uint8_t count;                                   // Current number of registered interrupts
} InterruptVectorTable;

typedef struct {
    uint8_t queue[IRQ_QUEUE_SIZE];  // Circular buffer for pending IRQ numbers
    uint8_t head;                   // Read index
    uint8_t tail;                   // Write index
    uint8_t count;                  // Number of pending interrupts
    pthread_mutex_t mutex;          // Mutex for thread safety
    pthread_cond_t cond;            // Condition variable to signal new interrupts
} InterruptQueue;

// ----------------------------
// Forward Declaration for UART
// ----------------------------
struct UART;  // Forward declaration so CPUState can hold a pointer to UART

// ----------------------------
// CPU State
// ----------------------------
typedef struct CPUState {
    PageTable *page_table;          // Pointer to the page table
    MemoryConfig memory_config;     // Memory configuration

    uint16_t* reg;
    uint32_t* pc;
    bool enable_mask_interrupts;
    bool z_flag;
    bool v_flag;

    InterruptQueue *i_queue;
    InterruptVectorTable *i_vector_table;

    struct UART *uart;              // Pointer to UART (full definition in uart.h)
    pthread_t uart_thread;

    // Display (for example, LCD dimensions defined in constants.h)
    char display[LCD_WIDTH][LCD_HEIGHT];
} CPUState;

// ----------------------------
// Application State
// ----------------------------
typedef struct AppState {
    char *program_file;
    char *flash_file;

    CPUState *state;

    uint8_t *emulator_running;
    pthread_t emulator_thread;

    size_t program_size;
    size_t flash_size;

    pid_t gui_pid;
    int gui_shm_fd;
} AppState;

#endif // COMMON_H
