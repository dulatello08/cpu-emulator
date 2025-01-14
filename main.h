#ifndef INC_16_BIT_CPU_EMULATOR_MAIN_H
#define INC_16_BIT_CPU_EMULATOR_MAIN_H

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
#include "constants.h"

// -- Page Table Definitions -- //
typedef struct PageTableEntry {
    uint8_t* page_data;                 // Points to physical memory data for this page
    bool is_allocated;                  // Indicates if the page is allocated
    uint32_t page_index;                // The logical index of this page (for address calculation)
    struct PageTableEntry* next;        // Pointer to the next page in the list
    struct PageTableEntry* prev;        // Pointer to the previous page in the list
} PageTableEntry;

typedef struct {
    PageTableEntry* head;               // Head of the doubly linked list
    PageTableEntry* tail;               // Tail of the doubly linked list
    size_t page_count;                  // Total number of pages in the table
} PageTable;

typedef enum {
    BOOT_SECTOR,
    USABLE_MEMORY,
    MMIO_PAGE,
    FLASH,
    USABLE_SECTOR,
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

typedef struct {
    uint8_t source;
    uint16_t handler;
} InterruptVectors;

// -- Interrupt Handling -- //
typedef struct {
    uint8_t* sources;       // Dynamic array to store interrupt sources
    uint8_t* size;          // Index of the top element
} InterruptQueue;

typedef struct {
    uint8_t sources[10];
    uint8_t size;
} GuiInterruptQueue;

// -- GUI Shared Memory -- //
typedef struct {
    char display[LCD_WIDTH][LCD_HEIGHT];
    uint8_t keyboard_o[2];
    GuiInterruptQueue i_queue;
} gui_process_shm_t;

// -- CPU State -- //
typedef struct {
    PageTable *page_table; // Pointer to the page table
    MemoryConfig memory_config;

    uint16_t* reg;
    uint32_t* pc;
    bool enable_mask_interrupts;
    bool z_flag;
    bool v_flag;

    char display[LCD_WIDTH][LCD_HEIGHT];
    InterruptVectors i_vector_table[INTERRUPT_TABLE_SIZE];
    InterruptQueue* i_queue;
} CPUState;

// -- Application State -- //
typedef struct {
    char *program_file;
    char *flash_file;

    CPUState *state;

    uint8_t *emulator_running;
    pthread_t emulator_thread;

    size_t program_size;
    size_t flash_size;

    gui_process_shm_t *gui_shm;
    pid_t gui_pid;
    int gui_shm_fd;
} AppState;

// -- Function Prototypes -- //

// Initialization and Start
int start(AppState *appState);
int parse_ini_file(const char *filename, MemoryConfig *config);

// CPU Execution and Memory Operations
bool execute_instruction(CPUState *state);
void increment_pc(CPUState *state, uint8_t opcode, uint8_t specifier);

// ALU Operations
void add(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint32_t operand2, uint8_t mode);
void subtract(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint32_t operand2, uint8_t mode);
void multiply(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint32_t operand2, uint8_t mode);
void left_shift(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint32_t operand2, uint8_t mode);
void right_shift(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint32_t operand2, uint8_t mode);
void bitwise_and(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint32_t operand2, uint8_t mode);
void bitwise_or(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint32_t operand2, uint8_t mode);
void bitwise_xor(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint32_t operand2, uint8_t mode);

// Page Table Management
PageTable* create_page_table(void);
PageTableEntry* allocate_page(PageTable *table, uint32_t page_index);
uint8_t* get_memory_ptr(CPUState *state, uint32_t address, bool allocate_if_unallocated);
uint16_t get_memory(CPUState *state, uint32_t address);
void set_memory(CPUState *state, uint32_t address, uint16_t value);
void bulk_copy_memory(CPUState *state, uint32_t address, const uint8_t *buffer, size_t length);
static inline void free_page(PageTable* table, PageTableEntry* page);
void initialize_page_table(CPUState *state, uint8_t *boot_sector_buffer, size_t boot_size);

void setupMmap(CPUState *state, size_t program_size);

// MMU and Stack Operations
void mmuControl(CPUState *state, uint8_t value);
void pushStack(CPUState *state, uint8_t value);
uint8_t popStack(CPUState *state, uint8_t *out);

// Display Operations
void clear_display(char display[LCD_WIDTH][LCD_HEIGHT]);
void print_display(char display[LCD_WIDTH][LCD_HEIGHT]);
void write_to_display(char display[LCD_WIDTH][LCD_HEIGHT], uint8_t data);

// Interrupt Management
void add_interrupt_vector(InterruptVectors table[INTERRUPT_TABLE_SIZE], uint8_t index, uint8_t source, uint16_t handler);
uint16_t get_interrupt_handler(const InterruptVectors table[INTERRUPT_TABLE_SIZE], uint8_t source);
void push_interrupt(InterruptQueue* queue, uint8_t source);
uint8_t pop_interrupt(InterruptQueue* queue);

// GUI Management
void open_gui(AppState *appState);
void handle_connection(int client_fd, CPUState *state, uint8_t *shared_data_memory);

// Utility
uint8_t count_leading_zeros(uint8_t x);
size_t load_program(const char *filename, uint8_t **buffer);

#endif //INC_16_BIT_CPU_EMULATOR_MAIN_H
