//
// Created by dulat on 2/17/23.
//
#include "main.h"

void setupMmap(CPUState *state, uint8_t program_size) {
    MemoryMap memoryMap = {
            .programMemory = { .startAddress = PROGRAM_MEMORY_START, .size = PROGRAM_MEMORY_SIZE(program_size) },
            .usableMemory = { .startAddress = USABLE_MEMORY_START(program_size), .size = USABLE_MEMORY_SIZE(program_size) },
            .flagsBlock = { .startAddress = FLAGS_START, .size = FLAGS_SIZE},
            .stackMemory = { .startAddress = STACK_START , .size = STACK_SIZE },
            .mmuControl = { .startAddress = MMU_CONTROL_START, .size = MMU_CONTROL_SIZE },
            .peripheralControl = { .startAddress = PERIPHERAL_CONTROL_START, .size = PERIPHERAL_CONTROL_SIZE },
            .flashControl = { .startAddress = FLASH_CONTROL_START, .size = FLASH_CONTROL_SIZE },
            .currentFlashBlock = { .startAddress = FLASH_BLOCK_START, .size = FLASH_BLOCK_SIZE }
    };
    state->mm = memoryMap;

    // Print out the program memory block
    printf("Program Memory:\n");
    printf("\tStart Address: 0x%04x\n", memoryMap.programMemory.startAddress);
    printf("\tSize: %x\n", memoryMap.programMemory.size);

    // Print out the usable memory block
    printf("Usable Memory:\n");
    printf("\tStart Address: 0x%04x\n", memoryMap.usableMemory.startAddress);
    printf("\tSize: %x\n", memoryMap.usableMemory.size);

    // Print out the flags block
    printf("Flags:\n");
    printf("\tStart Address: 0x%04x\n", memoryMap.flagsBlock.startAddress);
    printf("\tSize: %x\n", memoryMap.flagsBlock.size);

    // Print out the stack memory block
    printf("Stack Memory:\n");
    printf("\tStart Address: 0x%04x\n", memoryMap.stackMemory.startAddress);
    // Multi-stage stack ->> printf("\tStack top: 0x%04x\n", memoryMap.stackMemory.startAddress);
    printf("\t Size %x\n", memoryMap.stackMemory.size);

    // Print out the mmu control block
    printf("MMU Control:\n");
    printf("\tStart Address: 0x%04x\n", memoryMap.mmuControl.startAddress);
    printf("\tSize: %x\n", memoryMap.mmuControl.size);

    // Print out the peripheral control block
    printf("Peripheral Control:\n");
    printf("\tStart Address: 0x%04x\n", memoryMap.peripheralControl.startAddress);
    printf("\tSize: %x\n", memoryMap.peripheralControl.size);

    // Print out the flash control block
    printf("Memory Block:\n");
    printf("\tStart Address: 0x%04x\n", memoryMap.flashControl.startAddress);
    printf("\tSize: %x\n", memoryMap.flashControl.size);

    // Print out the current flash block
    printf("Current Flash Block:\n");
    printf("\tStart Address: 0x%04x\n", memoryMap.currentFlashBlock.startAddress);
    printf("\tSize: %x\n", memoryMap.currentFlashBlock.size);
}

bool handleWrite(CPUState *state, uint16_t address, uint8_t value) {
    //Return true if write permission is denied
    if (IS_ADDRESS_IN_RANGE(address, state->mm.programMemory)) {
        // Write to program memory
        return true;
    } else if (IS_ADDRESS_IN_RANGE(address, state->mm.usableMemory)) {
        // Write to usable memory
        return false;
    } else if (IS_ADDRESS_IN_RANGE(address, state->mm.flagsBlock)){
        return true;
    } else if(IS_ADDRESS_IN_RANGE(address, state->mm.stackMemory)) {
        // Write to stack
        return true;
    }else if (IS_ADDRESS_IN_RANGE(address, state->mm.mmuControl)) {
        // Write to MMU control
        mmuControl(state, value);
        return false;
    } else if (IS_ADDRESS_IN_RANGE(address, state->mm.peripheralControl)) {
        // Write to peripheral control
        if((address == state->mm.peripheralControl.startAddress) || (address == state->mm.peripheralControl.startAddress + 1)) {
            write_to_display(state->display, value);
        }
        return false;
    } else if (IS_ADDRESS_IN_RANGE(address, state->mm.flashControl)) {
        // Write to flash control
        return false;
    } else if (IS_ADDRESS_IN_RANGE(address, state->mm.currentFlashBlock)) {
        // Write to current flash block
        return false;
    } else {
        // Address is not within any known memory region
        printf("How? \xF0\x9F\x98\xAE\n");
        return true;
    }
}

void mmuControl(CPUState *state, uint8_t value) {
    switch(value) {
        case READ_PERIPHERAL_MMAP:;
            uint16_t startAddress = state->mm.peripheralControl.startAddress;
            uint8_t size = state->mm.peripheralControl.size;
            state->memory[state->mm.mmuControl.startAddress + 1] = startAddress >> 8;
            state->memory[state->mm.mmuControl.startAddress + 2] = startAddress;
            state->memory[state->mm.mmuControl.startAddress + 3] = size;
    }
}
