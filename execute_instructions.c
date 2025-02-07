//
// Created by dulat on 1/24/23.
//

#include "main.h"
#include <stdint.h>
#include <sys/stat.h>

bool execute_instruction(CPUState *state) {
    // Get a pointer to the current program counter (PC) address
    uint8_t *pc_ptr = get_memory_ptr(state, *(state->pc), false);
    if (!pc_ptr) {
        fprintf(stderr, "Invalid memory access at PC address 0x%08x\n", *(state->pc));
        return false;
    }
    //TODO!! NOT GONNA WORK ON PAGE BOUNDARY

    // Every instruction always has a specifier and an opcode:
    uint8_t specifier = pc_ptr[0];
    uint8_t opcode = pc_ptr[1];

    // Many instructions use a destination register.
    // (For instructions that are shorter than 3 bytes, this would be invalid.)
    uint8_t rd = pc_ptr[2];

    // Some instructions use a source register at offset 3.
    uint8_t rn = pc_ptr[3];

    // Some instructions use an immediate 16-bit value.
    // For example: "add %rd, #immediate" uses bytes 3 and 4.
    uint16_t immediate = ((uint16_t) pc_ptr[3] << 8) | pc_ptr[4];

    // Some instructions use a 32-bit value for normal addressing.
    // For example: "add %rd, [%normAddressing]" uses bytes 3,4,5,6.
    uint32_t normAddressing = ((uint32_t) pc_ptr[3] << 24) |
                              ((uint32_t) pc_ptr[4] << 16) |
                              ((uint32_t) pc_ptr[5] << 8) |
                              pc_ptr[6];

    // Branch instructions use a 32-bit label.
    // The "b" instruction (length 6) uses bytes 2-5.
    // uint32_t label_b = ((uint32_t) pc_ptr[2] << 24) |
    //                    ((uint32_t) pc_ptr[3] << 16) |
    //                    ((uint32_t) pc_ptr[4] << 8) |
    //                    pc_ptr[5];

    // Other branch instructions (be, bne, blt, bgt) use a 32-bit label from bytes 4-7.
    // uint32_t label_branch = ((uint32_t) pc_ptr[4] << 24) |
    //                         ((uint32_t) pc_ptr[5] << 16) |
    //                         ((uint32_t) pc_ptr[6] << 8) |
    //                         pc_ptr[7];

    // Some mov instruction variants use an offset.
    // For example, "mov %rd.L, [%rn + #offset]" (length 8) has an offset in bytes 4-7.
    uint32_t offset = ((uint32_t) pc_ptr[4] << 24) |
                      ((uint32_t) pc_ptr[5] << 16) |
                      ((uint32_t) pc_ptr[6] << 8) |
                      pc_ptr[7];

    // There are also variants with two register fields.
    // For example, "mov %rd, %rd1, [%rn + #offset]" (length 9)
    // Here: rd is at byte 2, rd1 is at byte 3, rn is at byte 4, and the offset is bytes 5-8.
    // uint8_t rd1 = pc_ptr[3]; // also used as rn1 in some variants
    uint8_t rn1 = pc_ptr[3]; // note: same offset as rd1 in some encodings
    /*uint32_t offset2 = ((uint32_t) pc_ptr[5] << 24) |
                       ((uint32_t) pc_ptr[6] << 16) |
                       ((uint32_t) pc_ptr[7] << 8) |
                       pc_ptr[8];*/

    bool skipIncrementPC = false; // Flag to skip incrementing the program counter

    switch (opcode) {
        case OP_NOP:
            // No operation, nothing to do.
            printf("Halt\n");
            return true;

        case OP_ADD:
            add(state, rd, rn, immediate, normAddressing, specifier);
            break;

        case OP_SUB:
            subtract(state, rd, rn, immediate, normAddressing, specifier);
            break;

        case OP_MUL:
            multiply(state, rd, rn, immediate, normAddressing, specifier);
            break;

        case OP_LSH:
            left_shift(state, rd, rn, immediate, normAddressing, specifier);
            break;

        case OP_RSH:
            right_shift(state, rd, rn, immediate, normAddressing, specifier);
            break;

        case OP_AND:
            bitwise_and(state, rd, rn, immediate, normAddressing, specifier);
            break;

        case OP_OR:
            bitwise_or(state, rd, rn, immediate, normAddressing, specifier);
            break;

        case OP_XOR:
            bitwise_xor(state, rd, rn, immediate, normAddressing, specifier);
            break;

        case OP_MOV:
            mov(state, rd, rn, rn1, immediate, normAddressing, offset, specifier);
            break;

        // Add additional opcodes here...
        default:
            printf("Unhandled opcode: %02x\n", opcode);
            break;
    }
    if (!skipIncrementPC) {
        increment_pc(state, opcode, specifier); // Increment the program counter if not skipped
    }
    return false;
}
