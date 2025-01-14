#include "main.h"
#include <curses.h>
#include <stdint.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

void increment_pc(CPUState *state, uint8_t opcode) {
    switch (opcode) {
        case OP_NOP:
        case OP_HLT:
        case OP_OSR:
        default:
            *(state->pc) += 1;
            break;
        case OP_CLZ:
        case OP_PSH:
        case OP_POP:
        case OP_RSM:
        case OP_RLD:
        case OP_AND:
        case OP_ORR:
        case OP_XOR:
            *(state->pc) += 2;
            break;
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_STO:
        case OP_BRN:
        case OP_BRZ:
        case OP_BRO:
        case OP_JSR:
        case OP_LSH:
        case OP_RSH:
        case OP_MULL:
            *(state->pc) += 3;
            break;
        case OP_ADM:
        case OP_SBM:
        case OP_MLM:
        case OP_ADR:
        case OP_SBR:
        case OP_MLR:
        case OP_STM:
        case OP_LDM:
        case OP_BRR:
        case OP_BNR:
        case OP_LSR:
        case OP_RSR:
            *(state->pc) += 4;
            break;
    }
}

void handle_operation(
    CPUState *state,
    uint8_t operand_rd,
    uint8_t operand_rn,
    uint32_t operand2,
    uint8_t mode,
    uint32_t (*operation)(uint8_t, uint16_t)
) {
    uint32_t result = 0;

    // Determine result based on mode using a switch-case.
    switch (mode) {
        case 0:
            // Mode 0: Use operand_rd and immediate operand2.
            result = operation(state->reg[operand_rd], operand2);
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

uint16_t add_operation(uint8_t operand1, uint16_t operand2) {
    return (uint16_t) operand1 + operand2;
}

uint16_t subtract_operation(uint8_t operand1, uint16_t operand2) {
    int16_t result = (int16_t) operand1 - (int16_t) operand2;
    if (result < 0) {
        return 0;
    }
    return (uint16_t) result;
}

uint16_t multiply_operation(uint8_t operand1, uint16_t operand2) {
    return (uint16_t) operand1 * operand2;
}

uint16_t left_shift_operation(uint8_t operand1, uint16_t operand2) {
    return ((uint16_t) operand1 << operand2) & 0xFF;
}

uint16_t right_shift_operation(uint8_t operand1, uint16_t operand2) {
    return ((uint16_t) operand1 >> operand2) & 0xFF;
}

uint16_t and_operation(uint8_t operand1, uint16_t operand2) {
    return (uint16_t) operand1 & operand2;
}

uint16_t or_operation(uint8_t operand1, uint16_t operand2) {
    return (uint16_t) operand1 | operand2;
}

uint16_t xor_operation(uint8_t operand1, uint16_t operand2) {
    return (uint16_t) operand1 ^ operand2;
}

void add(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode) {
    handle_operation(state, operand_rd, operand_rn, operand2, mode, add_operation);
}

void subtract(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode) {
    handle_operation(state, operand_rd, operand_rn, operand2, mode, subtract_operation);
}

void multiply(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode) {
    handle_operation(state, operand_rd, operand_rn, operand2, mode, multiply_operation);
}

void left_shift(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode) {
    handle_operation(state, operand_rd, operand_rn, operand2, mode, left_shift_operation);
}

void right_shift(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode) {
    handle_operation(state, operand_rd, operand_rn, operand2, mode, right_shift_operation);
}

void bitwise_and(CPUState *state, uint8_t operand_rd, uint8_t operand_rn) {
    handle_operation(state, operand_rd, operand_rn, 0, 3, and_operation);
}

void bitwise_or(CPUState *state, uint8_t operand_rd, uint8_t operand_rn) {
    handle_operation(state, operand_rd, operand_rn, 0, 3, or_operation);
}

void bitwise_xor(CPUState *state, uint8_t operand_rd, uint8_t operand_rn) {
    handle_operation(state, operand_rd, operand_rn, 0, 3, xor_operation);
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
