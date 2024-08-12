#include "main.h"
#include <curses.h>
#include <stdint.h>


size_t load_program(const char *program_file, uint8_t **program_memory) {
    if (program_file == NULL) {
        fprintf(stderr, "Error: input file not specified.\n");
        return 0;
    }

    FILE *fpi = fopen(program_file, "rb");
    if (fpi == NULL) {
        fprintf(stderr, "Error: Failed to open input program file.\n");
        return 0;
    }

    fseek(fpi, 0, SEEK_END);
    unsigned long size = ftell(fpi);

    if (size != EXPECTED_PROGRAM_WORDS * sizeof(uint8_t)) {
        fprintf(stderr, "Error: Input program file does not contain %d bytes. It contains %ld bytes\n", EXPECTED_PROGRAM_WORDS, size);
        fclose(fpi);
        return 0;
    }

    *program_memory = malloc(size * sizeof(uint8_t));
    if (*program_memory == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for program memory.\n");
        fclose(fpi);
        return 0;
    }

    fseek(fpi, 0, SEEK_SET);
    size_t num_read = fread(*program_memory, sizeof(uint8_t), size, fpi);
    if (num_read != size) {
        fprintf(stderr, "Error: Failed to read %ld bytes from input program file.\n", size);
        free(*program_memory);
        fclose(fpi);
        return 0;
    }

    fclose(fpi);
    return num_read;
}

long load_flash(const char *flash_file, FILE *fpf, uint8_t ***flash_memory) {
    if (flash_file == NULL) {
        fprintf(stderr, "Error: flash file not specified.\n");
        return 0;
    }

    fpf = fopen(flash_file, "r+b");
    if (fpf == NULL) {
        fprintf(stderr, "Error: Failed to open input flash file.\n");
        return 0;
    }

    fseek(fpf, 0, SEEK_END);
    // bad code
    long file_size, flash_size_total = file_size = ftell(fpf);
    int num_blocks = ((int) file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    *flash_memory = calloc(num_blocks, sizeof(uint8_t *));
    if (*flash_memory == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for flash memory.\n");
        fclose(fpf);
        return 0;
    }

    for (int i = 0; i < num_blocks; i++) {
        (*flash_memory)[i] = calloc(BLOCK_SIZE, sizeof(uint8_t));
        if ((*flash_memory)[i] == NULL) {
            fprintf(stderr, "Error: Failed to allocate memory for flash block %d.\n", i);
            for (int j = 0; j < i; j++) {
                free((*flash_memory)[j]);
            }
            free(*flash_memory);
            fclose(fpf);
            return 0;
        }
    }

    fseek(fpf, 0, SEEK_SET);
    __attribute__ ((unused)) int non_zero_count = 0;
    for (int i = 0; i < num_blocks; i++) {
        size_t bytes_to_read = (file_size > BLOCK_SIZE * (i + 1)) ? BLOCK_SIZE : (file_size % BLOCK_SIZE == 0 ? BLOCK_SIZE : file_size % BLOCK_SIZE);
        size_t num_read = fread((*flash_memory)[i], sizeof(uint8_t), bytes_to_read, fpf);
        if (num_read != bytes_to_read) {
            fprintf(stderr, "Error: Failed to read %ld bytes from input flash file for block %d.\n", bytes_to_read, i);
            for (int j = 0; j < num_blocks; j++) {
                free((*flash_memory)[j]);
            }
            free(*flash_memory);
            fclose(fpf);
            return 0;
        }
        for (int j = 0; j < (int) bytes_to_read; j++) {
            non_zero_count += (*flash_memory)[i][j] != 0;
        }
        file_size -= (int) bytes_to_read;
    }

    fclose(fpf);
    return flash_size_total;
}

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

void handle_operation(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode, uint16_t (*operation)(uint8_t, uint16_t)) {
    uint16_t result;
    // mode 0 & 1: result is stored at rd, 2 is at memory address operand2
    if(mode == 0) {
        result = operation(state->reg[operand_rd], operand2);
    } else if(mode == 1) {
        result = operation(state->reg[operand_rn], memory_access(state, 0, operand2, 0, 1));
    } else /*if(mode == 2)*/ {
        result = (uint8_t) operation(state->reg[operand_rd], state->reg[operand_rn]);
    }

    state->v_flag = (result > UINT8_MAX);
    state->z_flag = (result == 0);

    if(state->v_flag) {
        if(mode == 0 || mode == 1 || mode == 3) {
            state->reg[operand_rd] = UINT8_MAX;
        } else if (mode == 2) {
            memory_access(state, UINT8_MAX, operand2, 1, 1);
        }
    } else {
        if(mode == 0 || mode == 1 || mode == 3) {
            state->reg[operand_rd] = (uint8_t)result;
        } else if(mode == 2) {
            memory_access(state, result, operand2, 1, 1);
        }
    }
}

uint16_t add_operation(uint8_t operand1, uint16_t operand2) {
    return (uint16_t)operand1 + operand2;
}

uint16_t subtract_operation(uint8_t operand1, uint16_t operand2) {
    int16_t result = (int16_t)operand1 - (int16_t)operand2;
    if (result < 0) {
        return 0;
    }
    return (uint16_t)result;
}

uint16_t multiply_operation(uint8_t operand1, uint16_t operand2) {
    return (uint16_t)operand1 * operand2;
}

uint16_t left_shift_operation(uint8_t operand1, uint16_t operand2) {
    return (uint16_t)operand1 << operand2;
}

uint16_t right_shift_operation(uint8_t operand1, uint16_t operand2) {
    return (uint16_t)operand1 >> operand2;
}

uint16_t and_operation(uint8_t operand1, uint16_t operand2) {
    return (uint16_t)operand1 & operand2;
}

uint16_t or_operation(uint8_t operand1, uint16_t operand2) {
    return (uint16_t)operand1 | operand2;
}

uint16_t xor_operation(uint8_t operand1, uint16_t operand2) {
    return (uint16_t)operand1 ^ operand2;
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
    switch (mode) {
        case 0:
            // Read mode
            if(!srcDest) {
                state->reg[reg] = state->memory[address];
            }
            break;
        case 1:
            // Write mode
            if(!srcDest) {
                handleWrite(state, address, state->reg[reg]);
                state->memory[address] = state->reg[reg];
            } else {
                handleWrite(state, address, reg);
                state->memory[address] = reg;
            }
            break;
        default:
            break;
    }
    return state->memory[address];
}

void pushStack(CPUState *state, uint8_t value) {
    uint8_t stackTop = state->memory[state->mm.stackMemory.startAddress];

    // Shift existing values up by one position
    for (uint8_t i = stackTop; i > 0; i--) {
        state->memory[state->mm.stackMemory.startAddress + i + 1] = state->memory[state->mm.stackMemory.startAddress + i];
    }

    // Store the new value at the top of the stack
    state->memory[state->mm.stackMemory.startAddress + 1] = value;
    state->memory[state->mm.stackMemory.startAddress]++;
}

uint8_t popStack(CPUState *state, uint8_t *out) {
    uint8_t stackTop = state->memory[state->mm.stackMemory.startAddress];
    uint8_t value = state->memory[state->mm.stackMemory.startAddress + 1];

    // Shift values down by one position
    for (uint8_t i = 1; i < stackTop; i++) {
        state->memory[state->mm.stackMemory.startAddress + i] = state->memory[state->mm.stackMemory.startAddress + i + 1];
    }

    state->memory[state->mm.stackMemory.startAddress]--;

    if (out != NULL) {
        *out = value;
    }

    return value;
}


uint8_t count_leading_zeros(uint8_t x) {
    uint8_t count = 0;

    while (x != 0) {
        x >>= 1;
        count++;
    }

    return 8 - count;
}

bool hasChanged(int* lastValue, int currentValue) {
    bool changed = (*lastValue != currentValue);
    *lastValue = currentValue;
    return changed;
}
