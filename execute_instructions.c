//
// Created by dulat on 1/24/23.
//

#include "main.h"

bool execute_instruction(CPUState *state) {
    uint8_t opcode = state->memory[*(state->pc)];
    // might be unused
    uint16_t brnAddressing = state->memory[*(state->pc)+1] << 8 | state->memory[*(state->pc)+2];
    uint16_t normAddressing = state->memory[*(state->pc)+2] << 8 | state->memory[*(state->pc)+3];
    //uint8_t operand1 = state->memory[*(state->pc)+1];
    uint8_t operand_rd = (state->memory[*(state->pc)+1] >> 4) & 0xF;
    uint8_t operand_rn = state->memory[*(state->pc)+1] & 0xF;
    uint8_t operand2 = state->memory[*(state->pc)+2];
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
            if (state->reg[operand_rd] == 0) {
                state->z_flag = true;
            } else {
                state->z_flag = false;
            }
            break;
        // Store operand 2 in the operand Rd
        case OP_STO:
            state->reg[operand_rd] = operand2;
            break;
        // Store the value in the register Rd in the data memory at the operand 2
        case OP_STM:
            // Deprecated start
            if (normAddressing == 255) {
                printf("%02x\n", state->reg[operand_rd]);
            }
            // Deprecated end
            memory_access(state, operand_rd, normAddressing, 1, 0);
            break;
        // Load the value in the memory at the address in operand 2 into the register Rd
        case OP_LDM:
            memory_access(state, operand_rd, normAddressing, 0, 0);
            break;
        // Push the value in the register Rn at the specified address onto a stack
        case OP_PSH:
            pushStack(state, operand_rd);
            break;
        // Pop a value from the stack and store it in the register Rd
        case OP_POP:
            popStack(state, &(state->reg[operand_rd]));
            break;
        // Branch to value specified in operand 2
        case OP_BRN:
            *(state->pc) = brnAddressing;
            break;
        // Branch to value specified in operand2 if zero flag was set
        case OP_BRZ:
            if (state->z_flag) {
                *(state->pc) = brnAddressing;
            }
            break;
        // Branch to value specified in operand2 if overflow flag was not set.
        case OP_BRO:
            if (!state->v_flag) {
                *(state->pc) = brnAddressing;
            }
            break;
        // Branch to value specified in operand2 if register at operand 1 equals to opposite register
        case OP_BRR:
            if (state->reg[operand_rd]==state->reg[operand_rn]) {
                *(state->pc) = normAddressing;
            }
            break;
        // Branch to value specified in operand2 if register at operand 1 does not equal to opposite register
        case OP_BNR:
            if (state->reg[operand_rd] != state->reg[operand_rn]) {
                *(state->pc) = normAddressing;
            }
            break;
        // Halt
        case OP_HLT:
            printf("Halt at state of program counter: %d\n", *(state->pc));
            return true;
        // Create a new task, takes argument of memory address of the task's entry point. Insert the task into the task queue.
        case OP_TSK:
            state->reg[operand_rd] = create_task(state->task_queue, operand2);
            break;
        // Start the scheduler, should initialize the task queue, set the current task to the first task in the queue with kernel mode, and begin the scheduling loop
        case OP_SCH:
            initialize_scheduler(state->task_queue, (uint8_t *) state->pc);
            state->scheduler = true;
            break;
        // Switch to a specific task, takes argument of task's unique id. Update the task queue accordingly
        case OP_SWT:
            yield_task(state->task_queue, state->reg[operand_rd]);
            break;
        // Kill a specific task, takes argument of task's unique id. Remove the task from the task queue and free the memory allocated for the task.
        case OP_KIL:
            kill_task(state->task_queue, state->reg[operand_rd]);
            break;
        // Jump to subroutine at address of operand 1 and 2. Set inSubroutine flag to true.
        case OP_JSR:
            printf("before subroutine pc: %x\n", *state->pc);
            pushStack(state, *state->pc & 0xFF);
            pushStack(state, (*state->pc >> 8) & 0xFF);
            *(state->pc) = brnAddressing;
            *(state->inSubroutine) = true;
            break;
        // Jump out of subroutine use PC state saved in stack. Set inSubroutine flag to false.
        case OP_OSR:
            if(state->inSubroutine) {
                *(state->pc) = 0;
                uint8_t *temp = malloc(1 * sizeof(uint8_t));
                popStack(state, temp);
                *(state->pc) = *temp;
                popStack(state, temp);
                *(state->pc) += *temp << 8;
                *(state->pc)+=2;
                *(state->inSubroutine) = false;
                printf("current pc: %x \n", *state->pc);
                break;
            } else {
                printf("Jump out of subroutine was called while not in subroutine");
                return true;
            }
        // SIGILL
        default:
            printf("SIGILL: at state of program counter: %d\n", *(state->pc));
            printf("Instruction: %x was called\n", opcode);
            return true;
    }
    increment_pc(state, opcode);
    return false;
}