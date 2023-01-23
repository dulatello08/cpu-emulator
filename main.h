#ifndef INC_8_BIT_CPU_EMULATOR_MAIN_H
#define INC_8_BIT_CPU_EMULATOR_MAIN_H

#endif //INC_8_BIT_CPU_EMULATOR_MAIN_H

#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#define DATA_MEMORY 256
#define EXPECTED_PROGRAM_WORDS 255
#define EXPECTED_FLASH_WORDS 255
#define MAX_INPUT_LENGTH 1024

int start(const uint16_t *program_memory, uint8_t *data_memory, uint8_t *flash_memory);
void load_program(char *program_file, uint16_t **program_memory);
void load_flash(char *flash_file, FILE *fpf, uint8_t **flash_memory);