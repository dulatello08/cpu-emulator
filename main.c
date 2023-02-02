#include "main.h"
#include <stdint.h>
#include <sys/types.h>

void print_usage() {
    printf("Commands:\n");
    printf("start - start emulator\n");
    printf("program - load program\n");
    printf("flash - load flash\n");
    printf("help or h - display this help message\n");
    printf("exit - exit the program\n");
}

int main(int argc, char *argv[]) {
    char *program_file = NULL, *flash_file = NULL;
    char input[MAX_INPUT_LENGTH];
    uint8_t *program_memory = NULL;
    uint8_t *flash_memory = NULL;
    FILE *fpf = NULL;
    int input_len;
    uint8_t *shared_data_memory = mmap(NULL, DATA_MEMORY, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--program") == 0) {
            if (i + 1 < argc) {
                program_file = argv[i + 1];
                i++;
            }
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--flash") == 0) {
            if (i + 1 < argc) {
                flash_file = argv[i + 1];
                i++;
            }
        }
    }

    if (program_file) {
        load_program(program_file, &program_memory);
    }

    if (flash_file) {
        load_flash(flash_file, fpf, &flash_memory);
    }
    uint8_t emulator_running = 0;
    while(1) {
        printf(">> ");
        if (fgets(input, MAX_INPUT_LENGTH, stdin) == NULL) {
            printf("\n");
            break;
        }
        input_len = (int) strlen(input);
        if (input[input_len - 1] == '\n' && input[input_len - 2] == 8) {
            input[input_len - 2] = '\0';
        }
        else if (strcmp(input, "start\n") == 0) {
            if (emulator_running == 0) {
                pid_t emulator = fork();
                if(emulator==0) {
                    if(program_memory == NULL) {
                        printf("Program memory not loaded\n>> ");
                        exit(1);
                    }
                    if(flash_memory == NULL) {
                        printf("Flash memory not loaded\n>> ");
                        exit(1);
                    }
                    start(program_memory, shared_data_memory, flash_memory);
                    printf(">> ");
                    exit(0);
                }
                emulator_running = 1;
            } else {
                printf("Emulator already running.\n");
            }
        } else if (strncmp(input, "program ", 8) == 0) {
            char* filename = input + 8;
            filename[strcspn(filename, "\n")] = 0; // remove trailing newline character
            load_program(filename, &program_memory);
        } else if (strncmp(input, "flash ", 6) == 0) {
            char* filename = input + 6;
            filename[strcspn(filename, "\n")] = 0; // remove trailing newline character
            load_flash(filename, fpf, &flash_memory);
        } else if ((strcmp(input, "help\n") == 0) || (strcmp(input, "h\n") == 0)) {
            print_usage();
        } else if (strncmp(input, "input", 5) == 0) {
            char *arg = input + 6;
            char *endptr;
            unsigned long value = strtoul(arg, &endptr, 16);
            if (*endptr != '\n' || value > 0xff) {
                printf("Error: Invalid hexadecimal byte\n");
            } else {
                shared_data_memory[254] = (uint8_t)value;
            }
        } else if (strcmp(input, "free\n") == 0) {
            printf("Freeing emulator memory...\n");
            memset(shared_data_memory, 0, DATA_MEMORY);
        } else if (strcmp(input, "exit\n") == 0) {
            printf("Exiting emulator...\n");
            break;
        } else if (strcmp(input, "\n") == 0) {
            continue;
        }
        else {
            printf("Unknown command. Type help or h for help.\n");
        }
    }
    if(fpf!=NULL && flash_memory!=NULL) {
        fseek(fpf, 0, SEEK_SET);
        size_t num_written = fwrite(flash_memory, sizeof(uint8_t), EXPECTED_FLASH_WORDS, fpf);
        if (num_written != EXPECTED_FLASH_WORDS) {
            fprintf(stderr, "Error: Failed to write %d 8-bit words to output flash file.\n", EXPECTED_FLASH_WORDS);
        }
        fclose(fpf);
        free(flash_memory);
    }
    munmap(shared_data_memory, DATA_MEMORY);
    free(program_memory);
    return 0;
}
