//
// Created by dulat on 2/14/23.
//

#include "main.h"

#include <stdio.h>
#include <string.h>

int main() {
    char command[100];
    sprintf(command, "./emulator -p simple.m -m zero.bin");

    FILE *emulator = popen(command, "r");
    if (emulator == NULL) {
        printf("Failed to run emulator\n");
        return 1;
    }

    char output[100];
    while (fgets(output, sizeof(output), emulator) != NULL) {
        if (strstr(output, "Starting emulator") != NULL) {
            break;
        }
    }

    fprintf(emulator, "start\n");

    int result = 0;
    if (fgets(output, sizeof(output), emulator) != NULL) {
        sscanf(output, "%d", &result);
    }

    pclose(emulator);

    if (result != 4) {
        printf("Not passed\n");
        return 1;
    } else {
        printf("Passed\n");
        return 0;
    }
}