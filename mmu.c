//
// Created by dulat on 2/17/23.
//
#include "main.h"

#define PRINT_MEMORY_BLOCK(name, block) \
    printf("%s:\n", name);             \
    printf("\tStart Address: 0x%04x\n", block.startAddress); \
    printf("\tSize: %x\n", block.size);

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

    // print memory block
    PRINT_MEMORY_BLOCK("Program Memory", memoryMap.programMemory);
    PRINT_MEMORY_BLOCK("Usable Memory", memoryMap.usableMemory);
    PRINT_MEMORY_BLOCK("Flags", memoryMap.flagsBlock);
    PRINT_MEMORY_BLOCK("Stack Memory", memoryMap.stackMemory);
    PRINT_MEMORY_BLOCK("MMU Control", memoryMap.mmuControl);
    PRINT_MEMORY_BLOCK("Peripheral Control", memoryMap.peripheralControl);
    PRINT_MEMORY_BLOCK("Memory Block", memoryMap.flashControl);
    PRINT_MEMORY_BLOCK("Current Flash Block", memoryMap.currentFlashBlock);
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
        uint8_t ranged_address = address - state->mm.peripheralControl.startAddress;
        // Write to peripheral control
        if((ranged_address == 0x0) || (ranged_address == 0x1)) {
            write_to_display(state->display, value);
        } else if (ranged_address == 0x2) {
            // handle request for initializing interrupt vector table
            uint16_t ivt_start_address;
            ivt_start_address = (uint16_t) (popStack(state, NULL) << 8);
            ivt_start_address |= popStack(state, NULL);
            // Loop to read 30 bytes and store them in the IVT
            for (int i = 0; i < 30; i+=3) {
                uint8_t source = memory_access(state, 0, ivt_start_address + i, 0, 1);
                uint16_t handler = (uint16_t) (memory_access(state, 0, ivt_start_address + i + 1, 0, 1) << 8);
                handler |= memory_access(state, 0, ivt_start_address + i + 2, 0, 1);
                ///printf("IVT Start Address: %04x, Adding interrupt vector, source: %02x, handler %04x\n", ivt_start_address, source, handler);
                printf("Source: %02x, Handler preview: %02x %02x %02x %02x\n", source, state->memory[handler], state->memory[handler + 1], state->memory[handler + 2], state->memory[handler + 3]);
                add_interrupt_vector(state->i_vector_table, i/3, source, handler);
            }
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
