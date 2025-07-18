#include "main.h"
#include <curses.h>
#include <stdint.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

static uint8_t get_instruction_length(uint8_t opcode, uint8_t specifier) {
    switch (opcode) {
        case OP_NOP:
        case OP_HLT:
        case OP_RTS:
        case OP_WFI:
            return 2;
        case OP_PSH:
        case OP_POP:
            return 3;

        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_AND:
        case OP_OR:
        case OP_XOR:
        case OP_LSH:
        case OP_RSH:
        case OP_UMULL:
        case OP_SMULL:
            switch (specifier) {
                case 0x00: return 5;
                case 0x01:
                case 0x03:
                    return 4;
                case 0x02: return 7;
                default:   return 1;
            }

        case OP_MOV:
            switch (specifier) {
                case 0x00: return 5;
                case 0x01: return 8;
                case 0x02: return 4;
                case 0x03:
                case 0x04:
                case 0x05: return 7;
                case 0x06: return 8;
                case 0x07:
                case 0x08:
                case 0x09: return 7;
                case 0x0A:
                case 0x0B:
                case 0x0C:
                case 0x0D: return 8;
                case 0x0E: return 9;
                case 0x0F:
                case 0x10:
                case 0x11: return 8;
                case 0x12: return 9;
                default: return 1;
            }

        case OP_B:
        case OP_JSR:
            return 6;

        case OP_BE:
        case OP_BNE:
        case OP_BLT:
        case OP_BGT:
            return 8;

        default:
            return 1;
    }
}
/* Increments the CPUState's PC by the instruction's word length */
void increment_pc(CPUState *state, uint8_t opcode, uint8_t specifier) {
    *(state->pc) += get_instruction_length(opcode, specifier);
}

void handle_operation(
    CPUState *state,
    uint8_t operand_rd,
    uint8_t operand_rn,
    uint16_t immediate,
    uint32_t operand2,
    uint8_t mode,
    uint32_t (*operation)(uint16_t, uint32_t)
) {
    uint32_t result = 0;

    // Determine result based on mode using a switch-case.
    switch (mode) {
        case 0:
            // Mode 0: Use operand_rd and immediate operand2.
            result = operation(state->reg[operand_rd], immediate);
            break;
        case 1:
            // Mode 1: Use operand_rn, second operand as zero.
            result = operation(state->reg[operand_rn], state->reg[operand_rd]);
            break;
        case 2:
            // Mode 2: Use operand_rd and operand_rn.
            result = operation(state->reg[operand_rd], get_memory(state, operand2));
            break;
        default:
            printf("Unsupported mode: %d\n", mode);
            return;
    }

    // Update flags based on the result.
    state->v_flag = result > UINT16_MAX;
    state->z_flag = (result == 0);

    // Store the result or handle overflow based on mode.
    switch (mode) {
        case 0:
        case 1:
        case 2:
            // For modes 0, 1, and 2, store to register.
            state->reg[operand_rd] = (uint16_t) result;
            break;
        default:
            // Already handled unsupported modes above.
            break;
    }
}

uint32_t add_operation(uint16_t operand1, uint32_t operand2) {
    return (uint32_t) operand1 + operand2;
}

uint32_t subtract_operation(uint16_t operand1, uint32_t operand2) {
    int64_t result = (int64_t) operand1 - (int64_t) operand2;
    if (result < 0) {
        return 0;
    }
    return (uint32_t) result;
}

uint32_t multiply_operation(uint16_t operand1, uint32_t operand2) {
    return (uint32_t) operand1 * operand2;
}

uint32_t left_shift_operation(uint16_t operand1, uint32_t operand2) {
    return ((uint32_t) operand1 << operand2) & 0xFFFFFFFF;
}

uint32_t right_shift_operation(uint16_t operand1, uint32_t operand2) {
    return ((uint32_t) operand1 >> operand2) & 0xFFFFFFFF;
}

uint32_t and_operation(uint16_t operand1, uint32_t operand2) {
    return (uint32_t) operand1 & operand2;
}

uint32_t or_operation(uint16_t operand1, uint32_t operand2) {
    return (uint32_t) operand1 | operand2;
}

uint32_t xor_operation(uint16_t operand1, uint32_t operand2) {
    return (uint32_t) operand1 ^ operand2;
}

void add(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t immediate, uint32_t operand2, uint8_t mode) {
    handle_operation(state, operand_rd, operand_rn, immediate, operand2, mode, add_operation);
}

void subtract(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t immediate, uint32_t operand2,
              uint8_t mode) {
    handle_operation(state, operand_rd, operand_rn, immediate, operand2, mode, subtract_operation);
}

void multiply(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t immediate, uint32_t operand2,
              uint8_t mode) {
    handle_operation(state, operand_rd, operand_rn, immediate, operand2, mode, multiply_operation);
}

void left_shift(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t immediate, uint32_t operand2,
                uint8_t mode) {
    handle_operation(state, operand_rd, operand_rn, immediate, operand2, mode, left_shift_operation);
}

void right_shift(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t immediate, uint32_t operand2,
                 uint8_t mode) {
    handle_operation(state, operand_rd, operand_rn, immediate, operand2, mode, right_shift_operation);
}

void bitwise_and(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t immediate, uint32_t operand2,
                 uint8_t mode) {
    handle_operation(state, operand_rd, operand_rn, immediate, operand2, mode, and_operation);
}

void bitwise_or(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t immediate, uint32_t operand2,
                uint8_t mode) {
    handle_operation(state, operand_rd, operand_rn, immediate, operand2, mode, or_operation);
}

void bitwise_xor(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t immediate, uint32_t operand2,
                 uint8_t mode) {
    handle_operation(state, operand_rd, operand_rn, immediate, operand2, mode, xor_operation);
}

// Unsigned Multiply Long Long (UMULL)
void umull(uint16_t *rd, uint16_t *rn1, const uint16_t *rn) {
    uint32_t result = (uint32_t)(*rd) * (uint32_t)(*rn);  // Perform 16x16 unsigned multiplication
    *rd = (uint16_t)(result & 0xFFFF);   // Store lower 16 bits in rd
    *rn1 = (uint16_t)(result >> 16);     // Store upper 16 bits in rn1
}

// Signed Multiply Long Long (SMULL)
void smull(uint16_t *rd, uint16_t *rn1, const uint16_t *rn) {
    int32_t result = (int32_t)((int16_t)(*rd)) * (int32_t)((int16_t)(*rn));  // Perform 16x16 signed multiplication
    *rd = (uint16_t)(result & 0xFFFF);   // Store lower 16 bits in rd
    *rn1 = (uint16_t)(result >> 16);     // Store upper 16 bits in rn1
}


// Helper: Find the STACK memory section in the MemoryConfig.
// Returns a pointer to the STACK MemorySection or NULL if not found.
static MemorySection* find_stack_section(const MemoryConfig *config) {
    for (size_t i = 0; i < config->section_count; i++) {
        if (config->sections[i].type == STACK) {
            return (MemorySection *)&config->sections[i];
        }
    }
    return NULL;
}

/**
 * Push a single byte onto the stack.
 * The first 4 bytes of the stack section hold the current stack pointer.
 * The stack grows upward (i.e. increasing offset from the start of the data area).
 */
void pushStack(CPUState *state, uint8_t value) {
    MemorySection *stack_section = find_stack_section(&state->memory_config);
    if (!stack_section) {
        fprintf(stderr, "Error: Stack section not found in configuration.\n");
        exit(EXIT_FAILURE);
    }
    // The base address for the stack section in emulator memory.
    uint32_t base_addr = stack_section->start_address;
    // The first 4 bytes store the current stack pointer.
    uint8_t *sp_mem = get_memory_ptr(state, base_addr, true);
    if (!sp_mem) {
        fprintf(stderr, "Error: Unable to access stack pointer memory.\n");
        exit(EXIT_FAILURE);
    }
    uint32_t *sp_ptr = (uint32_t *)sp_mem;
    uint32_t sp = *sp_ptr; // current offset in the data area (in bytes)

    // Determine the maximum available bytes for stack data.
    // (Total section size minus 4 bytes reserved for the stack pointer)
    uint32_t total_section_bytes = stack_section->page_count * PAGE_SIZE;
    uint32_t max_stack_bytes = total_section_bytes - 4;
    if (sp >= max_stack_bytes) {
        fprintf(stderr, "Stack overflow: cannot push more data.\n");
        exit(EXIT_FAILURE);
    }

    // Calculate where to store the value:
    // (Base address + 4 bytes for pointer + current SP offset)
    uint32_t push_addr = base_addr + 4 + sp;
    uint8_t *dest = get_memory_ptr(state, push_addr, true);
    if (!dest) {
        fprintf(stderr, "Error: Unable to access memory for push operation.\n");
        exit(EXIT_FAILURE);
    }
    *dest = value;

    // Increment and update the stack pointer.
    sp++;
    *sp_ptr = sp;
}

/**
 * Pop a single byte from the stack.
 *
 * @param state Pointer to the CPU state.
 * @param out Pointer to a byte where the popped value will be stored.
 * @return 1 if a byte was successfully popped, or 0 if the stack was empty.
 */
uint8_t popStack(CPUState *state, uint8_t *out) {
    MemorySection *stack_section = find_stack_section(&state->memory_config);
    if (!stack_section) {
        fprintf(stderr, "Error: Stack section not found in configuration.\n");
        exit(EXIT_FAILURE);
    }
    uint32_t base_addr = stack_section->start_address;
    uint8_t *sp_mem = get_memory_ptr(state, base_addr, false);
    if (!sp_mem) {
        fprintf(stderr, "Error: Unable to access stack pointer memory.\n");
        return 0;
    }
    uint32_t *sp_ptr = (uint32_t *)sp_mem;
    uint32_t sp = *sp_ptr; // current offset (number of bytes on stack)

    if (sp == 0) {
        fprintf(stderr, "Stack underflow: no data to pop.\n");
        return 0;
    }

    // Decrement the stack pointer to point to the top-most data
    sp--;
    uint32_t pop_addr = base_addr + 4 + sp;
    uint8_t *src = get_memory_ptr(state, pop_addr, false);
    if (!src) {
        fprintf(stderr, "Error: Unable to access memory for pop operation.\n");
        return 0;
    }
    *out = *src;

    // Update the stored stack pointer.
    *sp_ptr = sp;
    return 1;
}

uint8_t count_leading_zeros(uint8_t x) {
    if (x == 0) return 8;
    uint8_t n = 0;
    if ((x & 0xF0) == 0) {
        n += 4;
        x <<= 4;
    }
    if ((x & 0xC0) == 0) {
        n += 2;
        x <<= 2;
    }
    if ((x & 0x80) == 0) { n += 1; }
    return n;
}

size_t load_program(const char *filename, uint8_t **buffer) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        *buffer = NULL;
        return 0;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        close(fd);
        *buffer = NULL;
        return 0;
    }

    size_t size = sb.st_size;
    *buffer = (uint8_t *) malloc(size);
    if (!*buffer) {
        close(fd);
        return 0;
    }

    ssize_t bytes_read = read(fd, *buffer, size);
    close(fd);

    if ((size_t) bytes_read != size) {
        free(*buffer);
        *buffer = NULL;
        return 0;
    }

    return size;
}
