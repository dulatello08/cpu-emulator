//
// Created by dulat on 1/24/23.
//

#include "main.h"
#include <stdint.h>

bool execute_instruction(CPUState *state) {
    uint8_t opcode = state->memory[*(state->pc)];

    //might be unused
    uint8_t operand_rd = (state->memory[*(state->pc) + 1] >> 4) & 0xF;
    uint8_t operand_rn = state->memory[*(state->pc) + 1] & 0xF;
    uint8_t operand2 = state->memory[*(state->pc) + 2];
    uint16_t brnAddressing = state->memory[*(state->pc) + 1] << 8 | state->memory[*(state->pc) + 2];
    uint16_t normAddressing = state->memory[*(state->pc) + 2] << 8 | state->memory[*(state->pc) + 3];

    bool skipIncrementPC = false;  // Flag to skip incrementing the program counter

    switch (opcode) {
        // Do nothing
        case OP_NOP:
            break;
        // Add operand 2 to the value in the operand Rd
        case OP_ADD:
            add(state, operand_rd, operand_rn, operand2, 0);
            break;
        // Subtract operand 2 from the value in the operand Rd
        case OP_SUB:
            subtract(state, operand_rd, operand_rn, operand2, 0);
            break;
        // Multiply the value in the operand Rd by operand 2
        case OP_MUL:
            multiply(state, operand_rd, operand_rn, operand2, 0);
            break;
        // Store sum of memory address at operand 2 and register Rn in register Rd
        case OP_ADM:
            add(state, operand_rd, operand_rn, normAddressing, 1);
            break;
        // Store difference of memory address at operand2 and register Rn in register Rd
        case OP_SBM:
            subtract(state, operand_rd, operand_rn, normAddressing, 1);
            break;
        // Multiply register Rn by memory address at operand 2 and store in register Rd
        case OP_MLM:
            multiply(state, operand_rd, operand_rn, normAddressing, 1);
            break;
        // Store sum of registers Rd and Rn in memory address at operand 2
        case OP_ADR:
            add(state, operand_rd, operand_rn, normAddressing, 2);
            break;
        // Store difference of registers Rd and Rn in memory address at operand 2
        case OP_SBR:
            subtract(state, operand_rd, operand_rn, normAddressing, 2);
            break;
        // Multiply registers Rd and Rn and store in memory address at operand 2
        case OP_MLR:
            multiply(state, operand_rd, operand_rn, normAddressing, 2);
            break;
        // Count the number of leading zeros at register Rn and store at Rd
        case OP_CLZ:
            state->reg[operand_rd] = count_leading_zeros(state->reg[operand_rn]);
            state->z_flag = (state->reg[operand_rd] == 0);
            break;
        // Store operand 2 in the operand Rd
        case OP_STO:
            state->reg[operand_rd] = operand2;
            break;
        // Store the value in the register Rd in the data memory at the operand 2
        case OP_STM:
            if (normAddressing == 255) {
                printf("%02x\n", state->reg[operand_rd]);
            }
            memory_access(state, operand_rd, normAddressing, 1, 0);
            break;
        // Load the value in the memory at the address in operand 2 into the register Rd
        case OP_LDM:
            memory_access(state, operand_rd, normAddressing, 0, 0);
            break;
        // Push the value in the register Rn at the specified address onto a stack
        case OP_PSH:
            pushStack(state, state->reg[operand_rd]);
            break;
        // Pop a value from the stack and store it in the register Rd
        case OP_POP:
            popStack(state, &(state->reg[operand_rd]));
            break;
        // Branch to value specified in operand 2
        case OP_BRN:
            //printf("%04x", brnAddressing);
            *(state->pc) = brnAddressing;
            skipIncrementPC = true;  // Skip incrementing the program counter
            break;
        // Branch to value specified in operand2 if zero flag was set
        case OP_BRZ:
            if (state->z_flag) {
                *(state->pc) = brnAddressing;
                skipIncrementPC = true;  // Skip incrementing the program counter
            }
            break;
        // Branch to value specified in operand2 if overflow flag was not set.
        case OP_BRO:
            if (!state->v_flag) {
                *(state->pc) = brnAddressing;
                skipIncrementPC = true;  // Skip incrementing the program counter
            }
            break;
        // Branch to value specified in operand2 if register at operand 1 equals to opposite register
        case OP_BRR:
            if (state->reg[operand_rd] == state->reg[operand_rn]) {
                *(state->pc) = normAddressing;
                printf("branched to %x", normAddressing);
                skipIncrementPC = true;  // Skip incrementing the program counter
            }
            break;
        // Branch to value specified in operand2 if register at operand 1 does not equal to opposite register
        case OP_BNR:
            if (state->reg[operand_rd] != state->reg[operand_rn]) {
                *(state->pc) = normAddressing;
                skipIncrementPC = true;  // Skip incrementing the program counter
            }
            break;
        // Halt
        case OP_HLT:
            printf("Halt at appState of program counter: %d\n", *(state->pc));
            return true;
        // Jump to subroutine at address of operand 1 and 2. Set in_subroutine flag to true.
        case OP_JSR:
            printf("before subroutine pc: %x\n", *state->pc);
            printf("subroutine jump to %x\n", brnAddressing);
            pushStack(state, *state->pc & 0xFF);
            pushStack(state, (*state->pc >> 8) & 0xFF);
            *(state->pc) = brnAddressing;
            *(state->in_subroutine) = true;
            skipIncrementPC = true;  // Skip incrementing the program counter
            break;
        // Jump out of subroutine using PC appState saved in stack. Set in_subroutine flag to false.
        case OP_OSR:
            if (state->in_subroutine) {
                uint16_t realPc;
                realPc = (uint16_t)(popStack(state, NULL) << 8);
                realPc |= popStack(state, NULL);
                *(state->pc) = realPc + 3;
                *(state->in_subroutine) = false;
                printf("current pc: %x \n", *state->pc);
                skipIncrementPC = true;
                break;
            } else {
                printf("Jump out of subroutine was called while not in subroutine");
                return true;
            }
        // Relative store register to memory pops off 2 bytes from stack to be used as address
        case OP_RSM: {
            uint16_t relAddr;
            relAddr = (uint16_t) (popStack(state, NULL) << 8);
            relAddr |= popStack(state, NULL);
            memory_access(state, operand_rd, relAddr, 1, 0);
            //printf("rsm relAddr: %04x\n", relAddr);
            break;
        }
        case OP_RLD: {
            uint16_t relAddr;
            relAddr = (uint16_t) (popStack(state, NULL) << 8);
            relAddr |= popStack(state, NULL);
            memory_access(state, operand_rd, relAddr, 0, 0);
            printf("rld relAddr: %04x\n", relAddr);
            break;
        }
        case OP_ENI:
            printf("enabling masked interrupts\n");
            state->enable_mask_interrupts = true;
            break;
        case OP_DSI:
            state->enable_mask_interrupts = false;
            break;

        // SIGILL
        default:
            printf("SIGILL: at state of program counter: %x\n", *(state->pc));
            printf("Instruction: %x was called\n", opcode);
            return true;
    }

    if (!skipIncrementPC) {
        increment_pc(state, opcode);  // Increment the program counter if not skipped
    }

    return false;
}
