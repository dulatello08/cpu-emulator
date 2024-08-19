//
// Created by Dulat S on 1/19/24.
//

#include "main.h"

// Define a struct for scan code mapping
typedef struct {
    int sdlCode;
    unsigned char cpuCode;
} ScanCodeMap;

// Define the mappings in a constant array
const ScanCodeMap SCAN_CODES[] = {
    {41, 0x01},
    {58, 0x02},
    {59, 0x03},
    {60, 0x04},
    {61, 0x05},
    {62, 0x06},
    {63, 0x07},
    {64, 0x08},
    {65, 0x09},
    {66, 0x0A},
    {67, 0x0B},
    {69, 0x0C},
    {42, 0x0D},
    {43, 0x0E},
    {57, 0x0F},
    {40, 0x10},
    {225, 0x11},
    {229, 0x12},
    {224, 0x13},
    {226, 0x14},
    {227, 0x15},
    {44, 0x16},
    {231, 0x17},
    {230, 0x18},
    {80, 0x19},
    {82, 0x1A},
    {79, 0x1B},
    {81, 0x1C},
    {4, 0x31},
    {5, 0x32},
    {6, 0x33},
    {7, 0x34},
    {8, 0x35},
    {9, 0x36},
    {10, 0x37},
    {11, 0x38},
    {12, 0x39},
    {13, 0x3A},
    {14, 0x3B},
    {15, 0x3C},
    {16, 0x3D},
    {17, 0x3E},
    {18, 0x3F},
    {19, 0x40},
    {20, 0x41},
    {21, 0x42},
    {22, 0x43},
    {23, 0x44},
    {24, 0x45},
    {25, 0x46},
    {26, 0x47},
    {27, 0x48},
    {28, 0x49},
    {29, 0x4A},
    {53, 0x4B},
    {30, 0x4C},
    {31, 0x4D},
    {32, 0x4E},
    {33, 0x4F},
    {34, 0x50},
    {35, 0x51},
    {36, 0x52},
    {37, 0x53},
    {38, 0x54},
    {39, 0x55},
    {45, 0x56},
    {46, 0x57},
    {47, 0x58},
    {48, 0x59},
    {49, 0x5A},
    {51, 0x5B},
    {52, 0x5C},
    {54, 0x5D},
    {55, 0x5E},
    {56, 0x5F},
    {0, 0}      // End marker
};

// Function to convert SDL scan code to CPU scan code using interpolation search
uint8_t sdlToCpuCode(int sdlCode) {
    int low = 0;
    int high = sizeof(SCAN_CODES) / sizeof(SCAN_CODES[0]) - 2; // exclude the last marker {0,0}

    while (low <= high && sdlCode >= SCAN_CODES[low].sdlCode && sdlCode <= SCAN_CODES[high].sdlCode) {
        if (low == high) {
            if (SCAN_CODES[low].sdlCode == sdlCode) {
                return SCAN_CODES[low].cpuCode;
            }
            break;
        }

        // Estimate the position
        int pos = low + (((double)(sdlCode - SCAN_CODES[low].sdlCode) /
                          (SCAN_CODES[high].sdlCode - SCAN_CODES[low].sdlCode)) * (high - low));

        // Check if the estimated position matches the sdlCode
        if (SCAN_CODES[pos].sdlCode == sdlCode) {
            return SCAN_CODES[pos].cpuCode;
        }

        // If sdlCode is larger, search in the upper part
        if (SCAN_CODES[pos].sdlCode < sdlCode) {
            low = pos + 1;
        } else {
            high = pos - 1;
        }
    }

    return 0; // Return 0 if no matching code is found
}