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

    // Fetch instruction and specifier directly
    uint8_t specifier = pc_ptr[0];       // First byte is the specifier
    uint8_t opcode = pc_ptr[1];          // Second byte is the opcode

    // Fetch two 8-bit register operands
    uint8_t operand_rd = pc_ptr[2];      // Third byte: destination register
    uint8_t operand_rn = pc_ptr[3];      // Fourth byte: source register

    // Fetch a 32-bit operand (composed of four bytes)
    uint32_t operand2 = ((uint32_t)pc_ptr[4] << 24) |
                        ((uint32_t)pc_ptr[5] << 16) |
                        ((uint32_t)pc_ptr[6] << 8) |
                        pc_ptr[7];

    // // Fetch a 32-bit branch addressing (composed of four bytes)
    // uint32_t brn_addressing = ((uint32_t)pc_ptr[8] << 24) |
    //                           ((uint32_t)pc_ptr[9] << 16) |
    //                           ((uint32_t)pc_ptr[10] << 8) |
    //                           pc_ptr[11];
    //
    // // Fetch a 32-bit normal addressing (composed of four bytes)
    // uint32_t norm_addressing = ((uint32_t)pc_ptr[12] << 24) |
    //                            ((uint32_t)pc_ptr[13] << 16) |
    //                            ((uint32_t)pc_ptr[14] << 8) |
    //                            pc_ptr[15];

    bool skipIncrementPC = false; // Flag to skip incrementing the program counter

    switch (opcode) {
        case OP_NOP:
            // No operation, nothing to do.
            break;

        case OP_ADD:
            add(state, operand_rd, operand_rn, operand2, specifier);
            break;

        case OP_SUB:
            subtract(state, operand_rd, operand_rn, operand2, specifier);
            break;

        case OP_MUL:
            multiply(state, operand_rd, operand_rn, operand2, specifier);
            break;

        case OP_LSH:
            left_shift(state, operand_rd, operand_rn, operand2, specifier);
            break;

        case OP_RSH:
            right_shift(state, operand_rd, operand_rn, operand2, specifier);
            break;

        case OP_AND:
            bitwise_and(state, operand_rd, operand_rn, operand2, specifier);
            break;

        case OP_OR:
            bitwise_or(state, operand_rd, operand_rn, operand2, specifier);
            break;

        case OP_XOR:
            bitwise_xor(state, operand_rd, operand_rn, operand2, specifier);
            break;

        // Add additional opcodes here...
        default:
            printf("Unhandled opcode: %02x\n", opcode);
            break;
    }
    //     // Count the number of leading zeros at register Rn and store at Rd
    //     case OP_CLZ:
    //         state->reg[operand_rd] = count_leading_zeros(state->reg[operand_rn]);
    //         state->z_flag = (state->reg[operand_rd] == 0);
    //         break;
    //     // Store operand 2 in the operand Rd
    //     case OP_STO:
    //         state->reg[operand_rd] = operand2;
    //         break;
    //     // Store the value in the register Rd in the data memory at the operand 2
    //     case OP_STM:
    //         if (normAddressing == 255) {
    //             printf("%02x\n", state->reg[operand_rd]);
    //         }
    //         memory_access(state, operand_rd, normAddressing, 1, 0);
    //         break;
    //     // Load the value in the memory at the address in operand 2 into the register Rd
    //     case OP_LDM:
    //         memory_access(state, operand_rd, normAddressing, 0, 0);
    //         break;
    //     // Push the value in the register Rn at the specified address onto a stack
    //     case OP_PSH:
    //         pushStack(state, state->reg[operand_rd]);
    //         break;
    //     // Pop a value from the stack and store it in the register Rd
    //     case OP_POP:
    //         popStack(state, (uint8_t *) &(state->reg[operand_rd]));
    //         break;
    //     // Branch to value specified in operand 2
    //     case OP_BRN:
    //         //printf("%04x", brnAddressing);
    //         *(state->pc) = brnAddressing;
    //         skipIncrementPC = true;  // Skip incrementing the program counter
    //         break;
    //     // Branch to value specified in operand2 if zero flag was set
    //     case OP_BRZ:
    //         if (state->z_flag) {
    //             *(state->pc) = brnAddressing;
    //             skipIncrementPC = true;  // Skip incrementing the program counter
    //         }
    //         break;
    //     // Branch to value specified in operand2 if overflow flag was not set.
    //     case OP_BRO:
    //         if (!state->v_flag) {
    //             *(state->pc) = brnAddressing;
    //             skipIncrementPC = true;  // Skip incrementing the program counter
    //         }
    //         break;
    //     // Branch to value specified in operand2 if register at operand 1 equals to opposite register
    //     case OP_BRR:
    //         if (state->reg[operand_rd] == state->reg[operand_rn]) {
    //             *(state->pc) = normAddressing;
    //             printf("branched to %x\n", normAddressing);
    //             skipIncrementPC = true;  // Skip incrementing the program counter
    //         }
    //         break;
    //     // Branch to value specified in operand2 if register at operand 1 does not equal to opposite register
    //     case OP_BNR:
    //         if (state->reg[operand_rd] != state->reg[operand_rn]) {
    //             *(state->pc) = normAddressing;
    //             skipIncrementPC = true;  // Skip incrementing the program counter
    //         }
    //         break;
    //     // Halt
    //     case OP_HLT:
    //         printf("Halt at appState of program counter: %d\n", *(state->pc));
    //         return true;
    //     // Jump to subroutine at address of operand 1 and 2.
    //     case OP_JSR:
    //         printf("before subroutine pc: %x\n", *state->pc);
    //         printf("subroutine jump to %x\n", brnAddressing);
    //         pushStack(state, *state->pc & 0xFF);
    //         pushStack(state, (*state->pc >> 8) & 0xFF);
    //         *(state->pc) = brnAddressing;
    //         skipIncrementPC = true;  // Skip incrementing the program counter
    //         break;
    //     // Jump out of subroutine using PC appState saved in stack.
    //     case OP_OSR: {
    //         uint16_t realPc;
    //         realPc = (uint16_t)(popStack(state, NULL) << 8);
    //         realPc |= popStack(state, NULL);
    //         *(state->pc) = realPc;
    //         printf("current pc: %x \n", *state->pc);
    //         skipIncrementPC = true;
    //         break;
    //     }
    //     // Relative store register to memory pops off 2 bytes from stack to be used as address
    //     case OP_RSM: {
    //         uint16_t relAddr;
    //         relAddr = (uint16_t) (popStack(state, NULL) << 8);
    //         relAddr |= popStack(state, NULL);
    //         memory_access(state, operand_rd, relAddr, 1, 0);
    //         //printf("rsm relAddr: %04x\n", relAddr);
    //         break;
    //     }
    //     case OP_RLD: {
    //         uint16_t relAddr;
    //         relAddr = (uint16_t) (popStack(state, NULL) << 8);
    //         relAddr |= popStack(state, NULL);
    //         memory_access(state, operand_rd, relAddr, 0, 0);
    //         printf("rld relAddr: %04x\n", relAddr);
    //         printf("rlded %c\n", state->reg[operand_rd]);
    //         break;
    //     }
    //     case OP_ENI:
    //         printf("enabling masked interrupts\n");
    //         state->enable_mask_interrupts = true;
    //         break;
    //     case OP_DSI:
    //         printf("disabling masked interrupts\n");
    //         state->enable_mask_interrupts = false;
    //         break;
    //
    //     // Left Shift
    //     case OP_LSH:
    //         left_shift(state, operand_rd, operand_rn, operand2, 0);
    //         break;
    //
    //     // Left Shift Register to Memory
    //     case OP_LSR:
    //         left_shift(state, operand_rd, operand_rn, normAddressing, 2);
    //         break;
    //
    //     // Right Shift
    //     case OP_RSH:
    //         right_shift(state, operand_rd, operand_rn, operand2, 0);
    //         break;
    //
    //     // Right Shift Register to Memory
    //     case OP_RSR:
    //         right_shift(state, operand_rd, operand_rn, normAddressing, 2);
    //         break;
    //
    //     // Bitwise AND
    //     case OP_AND:
    //         bitwise_and(state, operand_rd, operand_rn);
    //         break;
    //
    //     // Bitwise OR
    //     case OP_ORR:
    //         bitwise_or(state, operand_rd, operand_rn);
    //         break;
    //
    //     // Multiply long
    //     case OP_MULL: {
    //         printf("%02x\n", state->reg[1]);
    //         const uint16_t result = state->reg[(operand2 & 0xF0) >> 5] * state->reg[operand2 & 0x0F];
    //         state->reg[operand_rd] = (result & 0xFF00) >> 8;
    //         state->reg[operand_rn] = result & 0x00FF;
    //         break;
    //     }
    //
    //     // Bitwise XOR
    //
    //     case OP_XOR:
    //         bitwise_xor(state, operand_rd, operand_rn);
    //         break;
    //
    //     // SIGILL
    //     default:
    //         printf("SIGILL: at state of program counter: %x\n", *(state->pc));
    //         printf("Instruction: %x was called\n", opcode);
    //         return true;
    // }
    //
    if (!skipIncrementPC) {
        increment_pc(state, opcode, specifier); // Increment the program counter if not skipped
    }
    return true;
}
