#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "main.h"
#include <endian.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    int c;
    int help_flag = 0;
    char *input_file = NULL;
    char *flash_file = NULL;
    while ((c = getopt(argc, argv, "him:")) != -1) {
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
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                }
                return 1;
            default:
                abort();
        }
    }

    if (help_flag && input_file != NULL) {
        fprintf(stderr, "Error: Cannot specify both --help and --input.\n");
        return 1;
    }

    if (help_flag) {
        printf("Usage: emulator [-i <file>] [-i]\n");
        return 0;
    }

    if (input_file != NULL) {
        printf("Input file: %s\n", input_file);

        FILE *fpi = fopen(input_file, "rb");
        FILE *fpf = fopen(flash_file, "rb");
        if (fpi == NULL) {
            // Failed to open the file, return an error
            printf("Failed to open input file");
            return -1;
        }

        // Move the file pointer to the end of the file
        fseek(fpi, 0, SEEK_END);

        // Get the size of the file in bytes
        long size = ftell(fpi);

        // Calculate the number of 16-bit words in the file
        u_int64_t num_words = size / 2;

        if (num_words != 255) {
            // File has the wrong size, return an error
            printf("File does not contain 255 16-bit words\n");
            printf("It contains %lu of 16-bit words", num_words);
            fclose(fpi);
            return -1;
        }

         if (fpf == NULL) {
            // Failed to open the file, return an error
            printf("Failed to open input file");
            return -1;
        }

        // Move the file pointer to the end of the file
        fseek(fpf, 0, SEEK_END);

        // Get the size of the file in bytes
        long sizeF = ftell(fpi);

        // Calculate the number of 16-bit words in the file
        u_int64_t num_wordsF = sizeF / 2;

        if (num_wordsF != 255) {
            // File has the wrong size, return an error
            printf("File does not contain 255 16-bit words\n");
            printf("It contains %lu of 16-bit words", num_wordsF);
            fclose(fpi);
            return -1;
        }
        uint16_t *program_memory;
        program_memory = malloc(255*sizeof(uint16_t));
        fseek(fpi, 0, SEEK_SET);
        // Read the words from the file
        size_t num_read = fread(program_memory, sizeof(uint16_t), 255, fpi);
        if (num_read != 255) {
            // Failed to read the expected number of words, return an error
            free(program_memory);
            fclose(fpi);
            return -1;
        }
        *program_memory = ntohs(*program_memory);
        *program_memory = htobe16(*program_memory);
        uint8_t hi;
        start(program_memory, &hi);
        // File has the correct size, close the file and return success
        fclose(fpi);
    }

    return 0;
}
