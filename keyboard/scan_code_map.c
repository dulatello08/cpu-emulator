//
// Created by Dulat S on 1/5/24.
//

#include "main.h"

// Define a struct for scan code mapping
typedef struct {
    int sdlCode;
    unsigned char cpuCode;
} ScanCodeMap;

// Define the mappings in a constant array
const ScanCodeMap SCAN_CODES[] = {
    {41, 0x01}, // Escape
    {58, 0x02}, // F1
    {59, 0x03}, // F2
    // ... continue for other mappings
    {0, 0}      // End marker
};

// Function to convert SDL scan code to CPU scan code
unsigned char sdlToCpuCode(int sdlCode) {
    const ScanCodeMap *map = SCAN_CODES;
    while (map->sdlCode != 0) {
        if (map->sdlCode == sdlCode) {
            return map->cpuCode;
        }
        map++;
    }
    return 0; // Return 0 if no matching code is found
}
