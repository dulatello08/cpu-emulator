#include "main.h"
#include <stdint.h>
#include <sys/types.h>

int help_flag = 0;

void print_usage() {
    printf("Commands:\n");
    printf("start - start emulator\n");
    printf("program - load program\n");
    printf("flash - load flash\n");
    printf("help or h - display this help message\n");
    printf("exit - exit the program\n");
}

int main() {
    char input[MAX_INPUT_LENGTH];
    uint16_t *program_memory = NULL;
    uint8_t *flash_memory = NULL;
    FILE *fpf = NULL;
    uint8_t *shared_data_memory = mmap(NULL, DATA_MEMORY, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    while(1) {
        printf(">> ");
        if (fgets(input, MAX_INPUT_LENGTH, stdin) == NULL) {
            printf("\n");
            break;
        }
        if (strcmp(input, "start\n") == 0) {
            pid_t emulator = fork();
            if(emulator==0) {
                if(program_memory == NULL) {
                    printf("Program memory not loaded");
                    exit(1);
                }
                if(flash_memory == NULL) {
                    printf("Flash memory not loaded");
                    exit(1);
                }
                start(program_memory, shared_data_memory, flash_memory);
                printf(">> ");
                exit(0);
            }
        } else if (strcmp(input, "program\n") == 0) {
            printf("Enter program file name: ");
            if (fgets(input, MAX_INPUT_LENGTH, stdin) == NULL) {
                printf("\n");
                break;
            }
            
            // remove trailing newline character
            input[strcspn(input, "\n")] = 0;
            load_program(input, program_memory);
        } else if (strcmp(input, "flash\n")==0) {
            printf("Enter flash file name: ");
            if (fgets(input, MAX_INPUT_LENGTH, stdin) == NULL) {
                printf("\n");
                break;
            }
            // remove trailing newline character
            input[strcspn(input, "\n")] = 0;
            load_flash(input, fpf, flash_memory);
        } else if ((strcmp(input, "help\n") == 0) || (strcmp(input, "h\n") == 0)) {
            print_usage();
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
