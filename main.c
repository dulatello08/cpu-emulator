#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int c;
    int help_flag = 0;
    char *input_file = NULL;

    while ((c = getopt(argc, argv, "hi:")) != -1) {
        switch (c) {
            case 'h':
                help_flag = 1;
                break;
            case 'i':
                input_file = optarg;
                break;
            case '?':
                if (optopt == 'i') {
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
        printf("Usage: myprogram [--input <file>] [--help]\n");
        return 0;
    }

    if (input_file != NULL) {
        printf("Input file: %s\n", input_file);
    }

    return 0;
}
