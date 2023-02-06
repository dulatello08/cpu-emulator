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

void load_flash(char *flash_file, FILE *fpf, uint8_t **flash_memory) {
    if (flash_file == NULL) {
        fprintf(stderr, "Error: flash file not specified.\n");
        return;
    }

    fpf = fopen(flash_file, "r+b");
    if (fpf == NULL) {
        fprintf(stderr, "Error: Failed to open input flash file.\n");
        return;
    }

    fseek(fpf, 0, SEEK_END);
    long size = ftell(fpf);

    if (size != EXPECTED_FLASH_WORDS * sizeof(uint8_t)) {
        fprintf(stderr, "Error: Input flash file does not contain %d bytes. It contains %ld bytes.\n", EXPECTED_FLASH_WORDS, size);
        fclose(fpf);
        return;
    }

    *flash_memory = calloc(EXPECTED_FLASH_WORDS, sizeof(uint8_t));
    if (*flash_memory == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for flash memory.\n");
        fclose(fpf);
        return;
    }

    fseek(fpf, 0, SEEK_SET);
    size_t num_read = fread(*flash_memory, sizeof(uint8_t), EXPECTED_FLASH_WORDS, fpf);
    if (num_read != EXPECTED_FLASH_WORDS) {
        fprintf(stderr, "Error: Failed to read %d bytes from input flash file.\n", EXPECTED_FLASH_WORDS);
        free(*flash_memory);
        fclose(fpf);
        return;
    }

    fclose(fpf);
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
        if (state->data_memory[operand2] + state->reg[operand_rn] > UINT8_MAX) {
            state->v_flag = true;
        } else {
            state->v_flag = false;
            state->reg[operand_rd] = state->data_memory[operand2] + state->reg[operand_rn];
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
            state->data_memory[operand2] = state->reg[operand_rd] + state->reg[operand_rn];
            if (state->data_memory[operand2] == 0) {
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
        if (state->data_memory[operand2] < state->reg[operand_rn]) {
            state->v_flag = true;
        } else {
            state->v_flag = false;
            state->reg[operand_rd] = state->data_memory[operand2] - state->reg[operand_rn];
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
            state->data_memory[operand2] = state->reg[operand_rd] - state->reg[operand_rn];
            if (state->data_memory[operand2] == 0) {
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
        if (state->data_memory[operand2] * state->reg[operand_rn] > UINT8_MAX) {
            state->v_flag = true;
        } else {
            state->v_flag = false;
            state->reg[operand_rd] = state->data_memory[operand2] * state->reg[operand_rn];
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
            state->data_memory[operand2] = state->reg[operand_rd] * state->reg[operand_rn];
            if (state->data_memory[operand2] == 0) {
                state->z_flag = true;
            } else {
                state->z_flag = false;
            }
        }
    }
}