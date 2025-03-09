//
// main.h
// Created by Dulat S on 3/8/25.
//

#ifndef INC_16_BIT_CPU_EMULATOR_MAIN_H
#define INC_16_BIT_CPU_EMULATOR_MAIN_H

#include "common.h"

// ----------------------------
// Function Prototypes
// ----------------------------

// Initialization and Start
int start(AppState *appState);
int parse_ini_file(const char *filename, MemoryConfig *config);

// CPU Execution and Memory Operations
bool execute_instruction(CPUState *state);
void increment_pc(CPUState *state, uint8_t opcode, uint8_t specifier);

// ALU Operations
void add(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t immediate, uint32_t operand2, uint8_t mode);
void subtract(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t immediate, uint32_t operand2, uint8_t mode);
void multiply(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t immediate, uint32_t operand2, uint8_t mode);
void left_shift(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t immediate, uint32_t operand2, uint8_t mode);
void right_shift(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t immediate, uint32_t operand2, uint8_t mode);
void bitwise_and(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t immediate, uint32_t operand2, uint8_t mode);
void bitwise_or(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t immediate, uint32_t operand2, uint8_t mode);
void bitwise_xor(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t immediate, uint32_t operand2, uint8_t mode);

void umull(uint16_t *rd, uint16_t *rn1, const uint16_t *rn);
void smull(uint16_t *rd, uint16_t *rn1, const uint16_t *rn);

// Page Table Management
PageTable* create_page_table(void);
PageTableEntry* allocate_page(PageTable *table, uint32_t page_index);
uint8_t* get_memory_ptr(CPUState *state, uint32_t address, bool allocate_if_unallocated);
uint8_t get_memory(CPUState *state, uint32_t address);
void set_memory(CPUState *state, uint32_t address, uint8_t value);
void bulk_copy_memory(CPUState *state, uint32_t address, const uint8_t *buffer, size_t length);
void free_all_pages(PageTable* table);
void initialize_page_table(CPUState *state, uint8_t *boot_sector_buffer, size_t boot_size);

void setupMmap(CPUState *state, size_t program_size);

// MMU and Stack Operations
void mmuControl(CPUState *state, uint8_t value);
void pushStack(CPUState *state, uint8_t value);
uint8_t popStack(CPUState *state, uint8_t *out);

// ----------------------------
// Interrupt Management
// ----------------------------

// Interrupt Vector Table Functions
InterruptVectorTable* init_interrupt_vector_table(void);
bool register_interrupt_vector(InterruptVectorTable *table, uint8_t source, uint32_t handler_address);
bool unregister_interrupt_vector(InterruptVectorTable *table, uint8_t source);
InterruptVectorEntry* get_interrupt_vector(InterruptVectorTable *table, uint8_t source);

// Interrupt Queue Functions
InterruptQueue* init_interrupt_queue(void);
bool enqueue_interrupt(InterruptQueue *queue, uint8_t irq);
bool dequeue_interrupt(InterruptQueue *queue, uint8_t *irq);
bool is_interrupt_queue_empty(InterruptQueue *queue);
bool is_interrupt_queue_full(InterruptQueue *queue);

// ----------------------------
// Utility Functions
// ----------------------------
uint8_t count_leading_zeros(uint8_t x);
size_t load_program(const char *filename, uint8_t **buffer);

void mov(CPUState *state,
         uint8_t rd,
         uint8_t rn,
         uint8_t rn1,
         uint16_t immediate,
         uint32_t normAddress,
         uint32_t offset,
         uint8_t specifier);
void memory_write_trigger(CPUState *state, uint32_t address, uint32_t value);
uint8_t read8(CPUState* state, uint32_t address);
uint16_t read16(CPUState* state, uint32_t address);
uint32_t read32(CPUState* state, uint32_t address);
void write8(CPUState* state, uint32_t address, uint8_t value);
void write16(CPUState* state, uint32_t address, uint16_t value);
void write32(CPUState* state, uint32_t address, uint32_t value);

#endif // INC_16_BIT_CPU_EMULATOR_MAIN_H
