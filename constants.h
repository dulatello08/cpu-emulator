//
// Created by Dulat S on 10/30/24.
//

#ifndef NEOCORE_CONSTANTS_H
#define NEOCORE_CONSTANTS_H

// Emulator Configuration
#define MAX_INPUT_LENGTH 1024
#define PAGE_SIZE 4096
#define NUM_PAGES (1 << 20) // For a 32-bit address space and 4 KB pages

// CPU Operation Codes
#define OP_NOP 0x00
#define OP_ADD 0x01
#define OP_SUB 0x02
#define OP_MUL 0x03
#define OP_ADM 0x04
#define OP_SBM 0x05
#define OP_MLM 0x06
#define OP_ADR 0x07
#define OP_SBR 0x08
#define OP_MLR 0x09
#define OP_CLZ 0x0A
#define OP_STO 0x0B
#define OP_STM 0x0C
#define OP_LDM 0x0D
#define OP_PSH 0x0E
#define OP_POP 0x0F
#define OP_BRN 0x10
#define OP_BRZ 0x11
#define OP_BRO 0x12
#define OP_BRR 0x13
#define OP_BNR 0x14
#define OP_HLT 0x15
#define OP_JSR 0x16
#define OP_OSR 0x17
#define OP_RSM 0x18
#define OP_RLD 0x19
#define OP_ENI 0x1A
#define OP_DSI 0x1B
#define OP_LSH 0x1C
#define OP_LSR 0x1D
#define OP_RSH 0x1E
#define OP_RSR 0x1F
#define OP_AND 0x20
#define OP_ORR 0x21
#define OP_MULL 0x22
#define OP_XOR 0x23

// Memory Map Constants
#define FLAGS_SIZE 0x2
#define STACK_SIZE 0xff
#define MMU_CONTROL_SIZE 0x0004
#define PERIPHERAL_CONTROL_SIZE 0x8
#define FLASH_CONTROL_SIZE 0x1
#define FLASH_BLOCK_SIZE 0x1000
#define MEMORY 65536

// Address Macros
#define PROGRAM_MEMORY_START 0x0000
#define PROGRAM_MEMORY_SIZE(program_size) program_size
#define USABLE_MEMORY_START(program_size) (program_size)
#define USABLE_MEMORY_SIZE(program_size) (MEMORY - (FLAGS_SIZE + STACK_SIZE + MMU_CONTROL_SIZE + PERIPHERAL_CONTROL_SIZE + FLASH_CONTROL_SIZE + FLASH_BLOCK_SIZE + PROGRAM_MEMORY_SIZE(program_size)))
#define FLAGS_START (MEMORY - (FLAGS_SIZE + STACK_SIZE + MMU_CONTROL_SIZE + PERIPHERAL_CONTROL_SIZE + FLASH_CONTROL_SIZE + FLASH_BLOCK_SIZE))
#define STACK_START (MEMORY - (STACK_SIZE + MMU_CONTROL_SIZE + PERIPHERAL_CONTROL_SIZE + FLASH_CONTROL_SIZE + FLASH_BLOCK_SIZE))
#define MMU_CONTROL_START (MEMORY - (MMU_CONTROL_SIZE + PERIPHERAL_CONTROL_SIZE + FLASH_CONTROL_SIZE + FLASH_BLOCK_SIZE))
#define PERIPHERAL_CONTROL_START (MEMORY - (PERIPHERAL_CONTROL_SIZE + FLASH_CONTROL_SIZE + FLASH_BLOCK_SIZE))
#define FLASH_CONTROL_START (MEMORY - (FLASH_CONTROL_SIZE + FLASH_BLOCK_SIZE))
#define FLASH_BLOCK_START (FLASH_CONTROL_START + FLASH_CONTROL_SIZE)
#define IS_ADDRESS_IN_RANGE(address, region) ((address >= (region).startAddress) && (address < ((region).startAddress + (region).size)))

// Peripheral and Display Definitions
#define LCD_WIDTH 32
#define LCD_HEIGHT 4
#define READ_PERIPHERAL_MMAP 0x1

// Interrupt Definitions
#define INTERRUPT_TABLE_SIZE 10
#define INTERRUPT_QUEUE_MAX 10

#define MAX_SECTIONS 64

#endif //NEOCORE_CONSTANTS_H
