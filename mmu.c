//
// Created by dulat on 2/17/23.
//
#include "main.h"

void setupMmap(CPUState *state, uint8_t program_size) {
    MemoryMap memoryMap = {
            .programMemory = { .startAddress = PROGRAM_MEMORY_START, .size = PROGRAM_MEMORY_SIZE(program_size) },
            .usableMemory = { .startAddress = USABLE_MEMORY_START(program_size), .size = USABLE_MEMORY_SIZE(program_size) },
            .mmuControl = { .startAddress = MMU_CONTROL_START, .size = MMU_CONTROL_SIZE },
            .peripheralControl = { .startAddress = PERIPHERAL_CONTROL_START, .size = PERIPHERAL_CONTROL_SIZE },
            .flashControl = { .startAddress = FLASH_CONTROL_START, .size = FLASH_CONTROL_SIZE },
            .currentFlashBlock = { .startAddress = FLASH_BLOCK_START, .size = FLASH_BLOCK_SIZE }
    };
    state->mm = memoryMap;

    // Print out the program memory block
    printf("Program Memory:\n");
    printf("\tStart Address: 0x%04x\n", memoryMap.programMemory.startAddress);
    printf("\tSize: %d\n", memoryMap.programMemory.size);

    // Print out the usable memory block
    printf("Usable Memory:\n");
    printf("\tStart Address: 0x%04x\n", memoryMap.usableMemory.startAddress);
    printf("\tSize: %d\n", memoryMap.usableMemory.size);

    // Print out the mmu control block
    printf("MMU Control:\n");
    printf("\tStart Address: 0x%04x\n", memoryMap.mmuControl.startAddress);
    printf("\tSize: %d\n", memoryMap.mmuControl.size);

    // Print out the peripheral control block
    printf("Peripheral Control:\n");
    printf("\tStart Address: 0x%04x\n", memoryMap.peripheralControl.startAddress);
    printf("\tSize: %d\n", memoryMap.peripheralControl.size);

    // Print out the flash control block
    printf("Memory Block:\n");
    printf("\tStart Address: 0x%04x\n", memoryMap.flashControl.startAddress);
    printf("\tSize: %d\n", memoryMap.flashControl.size);

    // Print out the current flash block
    printf("Current Flash Block:\n");
    printf("\tStart Address: 0x%04x\n", memoryMap.currentFlashBlock.startAddress);
    printf("\tSize: %d\n", memoryMap.currentFlashBlock.size);
}