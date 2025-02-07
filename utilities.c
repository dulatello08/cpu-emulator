#include "main.h"
#include <curses.h>
#include <stdint.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

static uint8_t get_instruction_length(uint8_t opcode, uint8_t specifier) {
    switch (opcode) {
        case OP_NOP:
            return 2;

        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_AND:
        case OP_OR:
        case OP_XOR:
        case OP_LSH:
        case OP_RSH:
            switch (specifier) {
                case 0x00: return 5;
                case 0x01: return 4;
                case 0x02: return 7;
                default:   return 1;
            }

        case OP_MOV:
            switch (specifier) {
                case 0x00: return 5;
                case 0x01: return 4;
                case 0x02:
                case 0x03:
                case 0x04: return 7;
                case 0x05: return 8;
                case 0x06:
                case 0x07:
                case 0x08: return 7;
                case 0x09: return 8;
                case 0x0A:
                case 0x0B:
                case 0x0C: return 8;
                case 0x0D: return 9;
                case 0x0E:
                case 0x0F:
                case 0x10: return 8;
                case 0x11: return 9;
                default: return 1;
            }

        case OP_B:
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
    state->v_flag = (result > UINT16_MAX);
    state->z_flag = (result == 0);

    // Store the result or handle overflow based on mode.
    switch (mode) {
        case 0:
        case 1:
        case 2:
            // For modes 0, 1, and 2, store to register.
            if (state->v_flag) {
                state->reg[operand_rd] = UINT16_MAX;
            } else {
                state->reg[operand_rd] = (uint16_t) result;
            }
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

// This function performs a memory access.
//
// Parameters:
//   state: A pointer to the CPU state.
//   reg: The register to be accessed.
//   address: The memory address to be accessed.
//   mode: The access mode.
//   srcDest: The source or destination of the access.
//
// Returns:
//   The value of the memory location at the specified address.

uint8_t memory_access(CPUState *state, uint8_t reg, uint16_t address, uint8_t mode, uint8_t srcDest) {
    (void) state, (void) reg, (void) address, (void) srcDest;
    switch (mode) {
        case 0:
            // Read mode
            if (!srcDest) {
                //                state->reg[reg] = state->memory[address];
            }
            break;
        case 1:
            // Write mode
            if (!srcDest) {
                // handleWrite(state, address, state->reg[reg]);
                //                state->memory[address] = state->reg[reg];
            } else {
                // handleWrite(state, address, reg);
                //                state->memory[address] = reg;
            }
            break;
        default:
            break;
    }
    return 0;
}

void pushStack(CPUState *state, uint8_t value) {
    (void) state, (void) value;
    //    uint8_t stackTop = state->memory[state->mm.stackMemory.startAddress];

    // Shift existing values up by one position
    for (uint8_t i = 0; i > 0; i--) {
        //        state->memory[state->mm.stackMemory.startAddress + i + 1] = state->memory[state->mm.stackMemory.startAddress + i];
    }

    // Store the new value at the top of the stack
    //    state->memory[state->mm.stackMemory.startAddress + 1] = value;
    //    state->memory[state->mm.stackMemory.startAddress]++;
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
