#include "main.h"


void load_program(char *program_file, uint16_t **program_memory) {
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
        fprintf(stderr, "Error: Input program file does not contain %d 16-bit words.\n", EXPECTED_PROGRAM_WORDS);
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
        fprintf(stderr, "Error: Failed to read %d 16-bit words from input program file.\n", EXPECTED_PROGRAM_WORDS);
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

    if (size != EXPECTED_FLASH_WORDS) {
        fprintf(stderr, "Error: Input flash file does not contain %d 8-bit words.\n", EXPECTED_FLASH_WORDS);
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
        fprintf(stderr, "Error: Failed to read %d 8-bit words from input flash file.\n", EXPECTED_FLASH_WORDS);
        free(*flash_memory);
        fclose(fpf);
        return;
    }

    fclose(fpf);
}

void increment_pc(CPUState *state, int opcode) {
    switch (opcode) {
        case OP_NOP:
            state->pc++;
            break;
        case OP_ADD | OP_SUB | OP_MUL:
            state->pc += 2;
            break;
        case OP_ADM | OP_SBM | OP_MLM | OP_ADR | OP_SBR | OP_MLR:
            state->pc += 3;
            break;
        case OP_CLZ:
            state->pc += 2;
            break;
        case OP_STO | OP_STM | OP_LDM | OP_PSH:
            state->pc += 2;
            break;
        case OP_POP | OP_PRT:
            state->pc++;
            break;
        case OP_RDM | OP_RNM:
            state->pc += 3;
            break;
        case OP_BRN | OP_BRZ | OP_BRO:
            state->pc++;
            break;
        case OP_BRR | OP_BNR:
            state->pc += 3;
            break;
        case OP_HLT:
            break;
        default:
            printf("Error: invalid opcode\n");
            break;
    }
}