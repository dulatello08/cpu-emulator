//
// Created by dulat on 2/17/23.
//

#include "mmu.h"

void setupMmap(CPUState *state, uint8_t program_size) {
    MemoryMap memoryMap = {
            .programMemory = { .startAddress = PROGRAM_MEMORY_START, .size = PROGRAM_MEMORY_SIZE },
            .usableMemory = { .startAddress = USABLE_MEMORY_START, .size = USABLE_MEMORY_SIZE },
            .mmuControl = { .startAddress = MMU_CONTROL_START, .size = MMU_CONTROL_SIZE },
            .peripheralControl = { .startAddress = PERIPHERAL_CONTROL_START, .size = PERIPHERAL_CONTROL_SIZE },
            .memoryBlock = { .startAddress = 0, .size = 0 },
            .currentFlashBlock = { .startAddress = 0, .size = 0 }
    };
    state->mm
}