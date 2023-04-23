#include "main.h"
#include <stdint.h>
#include <sys/types.h>


uint8_t load_program(char *program_file, uint8_t **program_memory) {
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
    long size = ftell(fpi);

    if (size != EXPECTED_PROGRAM_WORDS * sizeof(uint8_t)) {
        fprintf(stderr, "Error: Input program file does not contain %d bytes. It contains %ld bytes\n", EXPECTED_PROGRAM_WORDS, size);
        fclose(fpi);
        return 0;
    }

    *program_memory = malloc(sizeof(uint8_t));
    if (*program_memory == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for program memory.\n");
        fclose(fpi);
        return 0;
    }

    fseek(fpi, 0, SEEK_SET);
    uint8_t *temp;
    temp = calloc(EXPECTED_PROGRAM_WORDS, sizeof(uint8_t));
    size_t num_read = fread(temp, sizeof(uint8_t), EXPECTED_PROGRAM_WORDS, fpi);
    if (num_read != EXPECTED_PROGRAM_WORDS) {
        fprintf(stderr, "Error: Failed to read %d bytes from input program file.\n", EXPECTED_PROGRAM_WORDS);
        free(*program_memory);
        fclose(fpi);
        return 0;
    }
    uint8_t current_byte = 0;
    while (temp[current_byte] != OP_HLT) {
        *program_memory = realloc(*program_memory, sizeof(uint8_t) * (current_byte + 1));
        memcpy(&(*program_memory)[current_byte], &temp[current_byte], 1);
        current_byte++;
    }
    *program_memory = realloc(*program_memory, sizeof(uint8_t) * (current_byte + 1));
    memcpy(&(*program_memory)[current_byte], &temp[current_byte], 1);
    current_byte++;
    fclose(fpi);
    return current_byte;
}

int load_flash(char *flash_file, FILE *fpf, uint8_t ***flash_memory) {
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
    long file_size = ftell(fpf);
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
    int non_zero_count = 0;
    for (int i = 0; i < num_blocks; i++) {
        size_t bytes_to_read = file_size > BLOCK_SIZE * (i + 1) ? BLOCK_SIZE : file_size % BLOCK_SIZE;
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
    return non_zero_count;
}

void increment_pc(CPUState *state, uint8_t opcode) {
    switch (opcode) {
        case OP_NOP:
        case OP_HLT:
        case OP_SCH:
        case OP_OSR:
        default:
            state->reg[15] += 1;
            break;
        case OP_CLZ:
        case OP_PSH:
        case OP_POP:
        case OP_SWT:
        case OP_KIL:
            state->reg[15] += 2;
            break;
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_STO:
        case OP_BRN:
        case OP_BRZ:
        case OP_BRO:
        case OP_JSR:
            state->reg[15] += 3;
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
        case OP_TSK:
            state->reg[15] += 4;
            break;
    }
}

void add(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode) {
    if (mode==0) {
        if (state->reg[operand_rd] + operand2 > UINT8_MAX) {
            state->v_flag = true;
            state->reg[operand_rd] = 0xFF;
        } else {
            state->v_flag = false;
            state->reg[operand_rd] += operand2;
            if (state->reg[operand_rd] == 0) {
                state->z_flag = true;
            } else {
                state->z_flag = false;
            }
        }
    } else if (mode==1) {
        if (memory_access(state, 0, operand2, 0, 1) + state->reg[operand_rn] > UINT8_MAX) {
            state->v_flag = true;
        } else {
            state->v_flag = false;
            state->reg[operand_rd] = memory_access(state, 0, operand2, 0, 1) + state->reg[operand_rn];
            if (state->reg[operand_rd] == 0) {
                state->z_flag = true;
            } else {
                state->z_flag = false;
            }
        }
    } else if (mode==2) {
        if (state->reg[operand_rd] + state->reg[operand_rn] > UINT8_MAX) {
            state->v_flag = true;
            state->reg[operand_rd] = 0xFF;
        } else {
            state->v_flag = false;
            memory_access(state, state->reg[operand_rd] + state->reg[operand_rn], operand2, 1, 1);
            if (state->memory[operand2] == 0) {
                state->z_flag = true;
            } else {
                state->z_flag = false;
            }
        }
    }
}

void subtract(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode) {
    if (mode == 0) {
        if (state->reg[operand_rd] < operand2) {
            state->v_flag = true;
            state->reg[operand_rd] = 0;
        } else {
            state->v_flag = false;
            state->reg[operand_rd] -= operand2;
            if (state->reg[operand_rd] == 0) {
                state->z_flag = true;
            } else {
                state->z_flag = false;
            }
        }
    } else if (mode == 1) {
        if (memory_access(state, 0, operand2, 0, 1) + state->reg[operand_rn] > UINT8_MAX) {
            state->v_flag = true;
        } else {
            state->v_flag = false;
            state->reg[operand_rd] = state->reg[operand_rn] - memory_access(state, 0, operand2, 0, 1);
            if (state->reg[operand_rd] == 0) {
                state->z_flag = true;
            } else {
                state->z_flag = false;
            }
        }
    } else if (mode == 2) {
        if (state->reg[operand_rd] < state->reg[operand_rn]) {
            state->v_flag = true;
            state->reg[operand_rd] = 0;
        } else {
            state->v_flag = false;
            memory_access(state, state->reg[operand_rd] - state->reg[operand_rn], operand2, 1, 1);
            if (state->memory[operand2] == 0) {
                state->z_flag = true;
            } else {
                state->z_flag = false;
            }
        }
    }
}

void multiply(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode) {
    if (mode==0) {
        if (state->reg[operand_rd] + operand2 > UINT8_MAX) {
            state->v_flag = true;
            state->reg[operand_rd] = 0xFF;
        } else {
            state->v_flag = false;
            state->reg[operand_rd] += operand2;
            if (state->reg[operand_rd] == 0) {
                state->z_flag = true;
            } else {
                state->z_flag = false;
            }
        }
    } else if (mode==1) {
        if (memory_access(state, 0, operand2, 0, 1) + state->reg[operand_rn] > UINT8_MAX) {
            state->v_flag = true;
        } else {
            state->v_flag = false;
            state->reg[operand_rd] = memory_access(state, 0, operand2, 0, 1) + state->reg[operand_rn];
            if (state->reg[operand_rd] == 0) {
                state->z_flag = true;
            } else {
                state->z_flag = false;
            }
        }
    } else if (mode==2) {
        if (state->reg[operand_rd] + state->reg[operand_rn] > UINT8_MAX) {
            state->v_flag = true;
            state->reg[operand_rd] = 0xFF;
        } else {
            state->v_flag = false;
            memory_access(state, state->reg[operand_rd] + state->reg[operand_rn], operand2, 1, 1);
            if (state->memory[operand2] == 0) {
                state->z_flag = true;
            } else {
                state->z_flag = false;
            }
        }
    }
}

uint8_t memory_access(CPUState *state, uint8_t reg, uint16_t address, uint8_t mode, uint8_t srcDest) {
    /*
     * reg argument can be used as value too
    */
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


void pushStack(CPUState *state, uint8_t reg) {
    uint16_t stackAddress = state->mm.stackMemory.startAddress;
    uint8_t regValue = state->reg[reg];
    memory_access(state, regValue, stackAddress, 1, 1);
}

uint8_t popStack(CPUState *state, uint8_t reg) {
    uint16_t stackAddress = state->mm.stackMemory.startAddress;
    uint8_t value = state->memory[stackAddress];
    state->reg[reg] = value;
    state->memory[stackAddress] = 0;
    return value;
}