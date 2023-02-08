#include "main.h"


void load_program(char *program_file, uint8_t **program_memory) {
    if (program_file == NULL) {
        fprintf(stderr, "Error: input file not specified.\n");
        return;
    }

    FILE *fpi = fopen(program_file, "rb");
    if (fpi == NULL) {
        fprintf(stderr, "Error: Failed to open input program file.\n");
        return;
    }

    fseek(fpi, 0, SEEK_END);
    long size = ftell(fpi);

    if (size != EXPECTED_PROGRAM_WORDS * sizeof(uint8_t)) {
        fprintf(stderr, "Error: Input program file does not contain %d bytes. It contains %ld bytes\n", EXPECTED_PROGRAM_WORDS, size);
        fclose(fpi);
        return;
    }

    *program_memory = calloc(EXPECTED_PROGRAM_WORDS, sizeof(uint8_t));
    if (*program_memory == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for program memory.\n");
        fclose(fpi);
        return;
    }

    fseek(fpi, 0, SEEK_SET);
    size_t num_read = fread(*program_memory, sizeof(uint8_t), EXPECTED_PROGRAM_WORDS, fpi);
    if (num_read != EXPECTED_PROGRAM_WORDS)
    {
        fprintf(stderr, "Error: Failed to read %d bytes from input program file.\n", EXPECTED_PROGRAM_WORDS);
        free(*program_memory);
        fclose(fpi);
        return;
    }

    fclose(fpi);
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
    int num_blocks = file_size / BLOCK_SIZE + (file_size % BLOCK_SIZE != 0);

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
        size_t bytes_to_read = (i == num_blocks - 1) ? file_size % BLOCK_SIZE : BLOCK_SIZE;
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
        for (int j = 0; j < bytes_to_read; j++) {
            non_zero_count += (*flash_memory)[i][j] != 0;
        }
    }

    fclose(fpf);
    return non_zero_count;
}

void increment_pc(CPUState *state, int opcode) {
    switch (opcode) {
        case OP_POP:
        case OP_PRT:
        case OP_BRN:
        case OP_BRZ:
        case OP_BRO:
        case OP_PSH:
        case OP_CLZ:
        case OP_SWT:
        case OP_KIL:
            *(state->pc) += 2;
            break;
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_STO:
        case OP_STM:
        case OP_LDM:
        case OP_ADM:
        case OP_SBM:
        case OP_MLM:
        case OP_ADR:
        case OP_SBR:
        case OP_MLR:
        case OP_RDM:
        case OP_RNM:
        case OP_BRR:
        case OP_BNR:
        case OP_TSK:
            *(state->pc) += 3;
            break;
        case OP_HLT:
        case OP_NOP:
        case OP_SCH:
        default:
            *(state->pc) += 1;
            break;
    }
}

void add(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint8_t operand2, uint8_t mode) {
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
        if (state->memory[operand2] + state->reg[operand_rn] > UINT8_MAX) {
            state->v_flag = true;
        } else {
            state->v_flag = false;
            state->reg[operand_rd] = state->memory[operand2] + state->reg[operand_rn];
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
            state->memory[operand2] = state->reg[operand_rd] + state->reg[operand_rn];
            if (state->memory[operand2] == 0) {
                state->z_flag = true;
            } else {
                state->z_flag = false;
            }
        }
    }
}

void subtract(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint8_t operand2, uint8_t mode) {
    if (mode==0) {
        if (state->reg[operand_rd] < operand2) {
            state->v_flag = true;
        } else {
            state->v_flag = false;
            state->reg[operand_rd] -= operand2;
            if (state->reg[operand_rd] == 0) {
                state->z_flag = true;
            } else {
                state->z_flag = false;
            }
        }
    } else if (mode==1) {
        if (state->memory[operand2] < state->reg[operand_rn]) {
            state->v_flag = true;
        } else {
            state->v_flag = false;
            state->reg[operand_rd] = state->memory[operand2] - state->reg[operand_rn];
            if (state->reg[operand_rd] == 0) {
                state->z_flag = true;
            } else {
                state->z_flag = false;
            }
        }
    } else if (mode==2) {
        if (state->reg[operand_rd] < state->reg[operand_rn]) {
            state->v_flag = true;
        } else {
            state->v_flag = false;
            state->memory[operand2] = state->reg[operand_rd] - state->reg[operand_rn];
            if (state->memory[operand2] == 0) {
                state->z_flag = true;
            } else {
                state->z_flag = false;
            }
        }
    }
}

void multiply(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint8_t operand2, uint8_t mode) {
    if (mode==0) {
        if (state->reg[operand_rd] * operand2 > UINT8_MAX) {
            state->v_flag = true;
            state->reg[operand_rd] = 0xFF;
        } else {
            state->v_flag = false;
            state->reg[operand_rd] *= operand2;
            if (state->reg[operand_rd] == 0) {
                state->z_flag = true;
            } else {
                state->z_flag = false;
            }
        }
    } else if (mode==1) {
        if (state->memory[operand2] * state->reg[operand_rn] > UINT8_MAX) {
            state->v_flag = true;
        } else {
            state->v_flag = false;
            state->reg[operand_rd] = state->memory[operand2] * state->reg[operand_rn];
            if (state->reg[operand_rd] == 0) {
                state->z_flag = true;
            } else {
                state->z_flag = false;
            }
        }
    } else if (mode==2) {
        if (state->reg[operand_rd] * state->reg[operand_rn] > UINT8_MAX) {
            state->v_flag = true;
            state->reg[operand_rd] = 0xFF;
        } else {
            state->v_flag = false;
            state->memory[operand2] = state->reg[operand_rd] * state->reg[operand_rn];
            if (state->memory[operand2] == 0) {
                state->z_flag = true;
            } else {
                state->z_flag = false;
            }
        }
    }
}