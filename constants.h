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
#define OP_NOP  0x00
#define OP_ADD  0x01
#define OP_SUB  0x02
#define OP_MUL  0x03
#define OP_AND  0x04
#define OP_OR   0x05
#define OP_XOR  0x06
#define OP_LSH  0x07
#define OP_RSH  0x08
#define OP_MOV  0x09
#define OP_B  0x0A
#define OP_BE  0x0B
#define OP_BNE  0x0C
#define OP_BLT  0x0D
#define OP_BGT  0x0E
#define OP_BRO  0x0F
#define OP_UMULL 0x10
#define OP_SMULL 0x11
#define OP_HLT 0x12

// Peripheral and Display Definitions
#define LCD_WIDTH 32
#define LCD_HEIGHT 4
#define READ_PERIPHERAL_MMAP 0x1

// Interrupt Definitions
#define INTERRUPT_TABLE_SIZE 10
#define INTERRUPT_QUEUE_MAX 10

#define MAX_SECTIONS 64

#endif //NEOCORE_CONSTANTS_H
