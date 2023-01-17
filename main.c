#include "main.h"

#define EXPECTED_PROGRAM_WORDS 255
#define EXPECTED_FLASH_WORDS 255

int help_flag = 0;

void print_usage() {
    printf("Usage: program -i <input file> -m <flash file>\n");
}

int main(int argc, char **argv) {
    int c;
    char *input_file = NULL;
    char *flash_file = NULL;

    while ((c = getopt(argc, argv, "hi:m:")) != -1) {
        switch (c) {
            case 'h':
                help_flag = 1;
                break;
            case 'i':
                input_file = optarg;
                break;
            case 'm':
                flash_file = optarg;
                break;
            case '?':
                if (optopt == 'i' || optopt == 'm') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option -%c.\n", optopt);
                }
                return 1;
            default:
                fprintf(stderr, "Error: Unknown option -%c.\n", optopt);
                return 1;
        }
    }

    if (help_flag) {
        print_usage();
        return 0;
    }

    if (input_file == NULL) {
        fprintf(stderr, "Error: input file not specified.\n");
        return 1;
    }

    FILE *fpi = fopen(input_file, "rb");
    if (fpi == NULL) {
        fprintf(stderr, "Error: Failed to open input program file.\n");
        return 1;
    }

    fseek(fpi, 0, SEEK_END);
    long size = ftell(fpi);

    if (size != EXPECTED_PROGRAM_WORDS * sizeof(uint16_t)) {
        fprintf(stderr, "Error: Input program file does not contain %d 16-bit words.\n", EXPECTED_PROGRAM_WORDS);
        fclose(fpi);
        return 1;
    }

    uint16_t *program_memory = calloc(EXPECTED_PROGRAM_WORDS, sizeof(uint16_t));
    if (program_memory == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for program memory.\n");
        fclose(fpi);
        return 1;
    }

    fseek(fpi, 0, SEEK_SET);
    size_t num_read = fread(program_memory, sizeof(uint16_t), EXPECTED_PROGRAM_WORDS, fpi);
    if (num_read != EXPECTED_PROGRAM_WORDS) {
        fprintf(stderr, "Error: Failed to read %d 16-bit words from input program file.\n", EXPECTED_PROGRAM_WORDS);
        free(program_memory);
        fclose(fpi);
        return 1;
    }

    fclose(fpi);

    if (flash_file == NULL) {
        fprintf(stderr, "Error: flash file not specified.\n");
        return 1;
    }

    FILE *fpf = fopen(flash_file, "r+b");
    if (fpf == NULL) {
        fprintf(stderr, "Error: Failed to open input flash file.\n");
        return 1;
    }

    fseek(fpf, 0, SEEK_END);
    size = ftell(fpf);

    if (size != EXPECTED_FLASH_WORDS) {
        fprintf(stderr, "Error: Input flash file does not contain %d 8-bit words.\n", EXPECTED_FLASH_WORDS);
        fclose(fpf);
        return 1;
    }

    uint8_t *flash_memory = calloc(EXPECTED_FLASH_WORDS, sizeof(uint8_t));
    if (flash_memory == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for flash memory.\n");
        fclose(fpf);
        return 1;
    }

    fseek(fpf, 0, SEEK_SET);
    num_read = fread(flash_memory, sizeof(uint8_t), EXPECTED_FLASH_WORDS, fpf);
    if (num_read != EXPECTED_FLASH_WORDS) {
        fprintf(stderr, "Error: Failed to read %d 8-bit words from input flash file.\n", EXPECTED_FLASH_WORDS);
        free(flash_memory);
        fclose(fpf);
        return 1;
    }
    pid_t pid = fork();
    if(pid == 0) {
        start(program_memory, flash_memory);
        exit(0);
    } else {
        char input[100];
        for (int i=0; i++, i==50;) {
            printf("Enter a command: ");
            fgets(input, 100, stdin);
            printf("\nCommand you entered: %s", input);
        }
    }

    fseek(fpf, 0, SEEK_SET);
    size_t num_written = fwrite(flash_memory, sizeof(uint8_t), EXPECTED_FLASH_WORDS, fpf);
    if (num_written != EXPECTED_FLASH_WORDS) {
        fprintf(stderr, "Error: Failed to write %d 8-bit words to output flash file.\n", EXPECTED_FLASH_WORDS);
        free(program_memory);
        free(flash_memory);
        return 1;
    }

    fclose(fpf);
    free(program_memory);
    free(flash_memory);
    return 0;
}
