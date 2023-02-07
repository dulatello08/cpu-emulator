//
// Created by dulat on 1/24/23.
//

#include "main.h"

bool execute_instruction(CPUState *state) {
    uint8_t opcode = state->memory[*state->pc];
    // might be unused
    uint8_t operand1 = state->memory[*state->pc+1];
    uint8_t operand_rd = (state->memory[*state->pc+1] >> 4) & 0xF;
    uint8_t operand_rn = state->memory[*state->pc+1] & 0xF;
    uint8_t operand2 = state->memory[*state->pc+2];
    switch (opcode) {
        // Do nothing
        case 0x00:
            break;
        // Add operand 2 to the value in the operand Rd
        case 0x01:
            add(state, operand_rd, operand_rn, operand2, 0);
            break;
        // Subtract operand 2 from the value in the operand Rd
        case 0x02:
            subtract(state, operand_rd, operand_rn, operand2, 0);
            break;
        // Multiply the value in the operand Rd by operand 2
        case 0x03:
            multiply(state, operand_rd, operand_rn, operand2, 0);
            break;
        // Store sum of memory address at operand 2 and register Rn in register Rd
        case 0x04:
            add(state, operand_rd, operand_rn, operand2, 1);
            break;
        // Store difference of memory address at operand2 and register Rn in register Rd
        case 0x05:
            subtract(state, operand_rd, operand_rn, operand2, 1);
            break;
        // Multiply register Rn by memory address at operand 2 and store in register Rd
        case 0x06:
            multiply(state, operand_rd, operand_rn, operand2, 1);
            break;
        // Store sum of registers Rd and Rn in memory address at operand 2
        case 0x07:
            add(state, operand_rd, operand_rn, operand2, 2);
            break;
        // Store difference of registers Rd and Rn in memory address at operand 2
        case 0x08:
            subtract(state, operand_rd, operand_rn, operand2, 2);
            break;
        // Multiply registers Rd and Rn and store in memory address at operand 2
        case 0x09:
            multiply(state, operand_rd, operand_rn, operand2, 2);
            break;
        // Count the number of leading zeros at register Rn and store at Rd
        case 0x0A:
            state->reg[operand_rd] = count_leading_zeros(state->reg[operand_rn]);
            if (state->reg[operand_rd] == 0) {
                state->z_flag = true;
            } else {
                state->z_flag = false;
            }
            break;
        // Store operand 2 in the operand Rd
        case 0x0B:
            state->reg[operand_rd] = operand2;
            break;
        // Store the value in the register Rd in the data memory at the operand 2
        case 0x0C:
            if (operand2 == 255) {
                printf("%02x\n", state->reg[operand_rd]);
            }
            state->memory[operand2] = state->reg[operand_rd];
            break;
        // Load the value in the memory at the address in operand 2 into the register Rd
        case 0x0D:
            state->reg[operand_rd] = state->memory[operand2];
            break;
        // Push the value in the register Rn at the specified address onto a stack
        case 0x0E:
            push(&state->ssr, state->reg[operand_rd]);
            break;
        // Pop a value from the stack and store it in the register Rd
        case 0x0F:
            state->reg[operand_rd] = pop(&state->ssr);
            break;
        // Move data from one memory address to other, source: Rd, destination: Operand2, print if destination 0xFF
        case 0x10:
            if (operand2 == 255) {
                printf("%02x\n", state->reg[operand_rd]);
            }
            state->memory[operand2] = state->memory[state->reg[operand_rd]];
            break;

        case 0x11: {
            // Read flash memory to data memory
            // Rd has address in data memory, Rn has address in flash memory
            // Copies single byte to data memory
            memcpy((void *) &state->memory[state->reg[operand_rd]], (void *) &state->memory[state->reg[operand_rn]], sizeof(uint8_t));
        }
            break;

        case 0x12: {
            // Read data memory to flash memory
            // Rd has address in flash memory, Rn has address in data memory
            // Copies single byte to flash memory
            memcpy((void *) &state->memory[state->reg[operand_rd]], (void *) &state->memory[state->reg[operand_rn]], sizeof(uint8_t));
        }
            break;
        // Branch to value specified in operand 2
        case 0x13:
            *(state->pc) = operand1;
            break;
        // Branch to value specified in operand2 if zero flag was set
        case 0x14:
            if (state->z_flag) {
                *(state->pc) = operand1;
            }
            break;
        // Branch to value specified in operand2 if overflow flag was not set.
        case 0x15:
            if (!state->v_flag) {
                *(state->pc) = operand1;
            }
            break;
        // Branch to value specified in operand2 if register at operand 1 equals to opposite register
        case 0x16:
            if (state->reg[operand_rd]==state->reg[operand_rn]) {
                *(state->pc) = operand1;
            }
            break;
        // Branch to value specified in operand2 if register at operand 1 does not equal to opposite register
        case 0x17:
            if (state->reg[operand_rd] != state->reg[operand_rn]) {
                *(state->pc) = operand1;
            }
            break;
        // Halt
        case 0x18:
            printf("Halt at state of program counter: %d\n", *state->pc);
            return true;
        // Create a new task, takes argument of memory address of the task's entry point. Insert the task into the task queue.
        case 0x19:
            state->reg[operand_rd] = create_task(state->task_queue, operand2);
            break;
        // Start the scheduler, should initialize the task queue, set the current task to the first task in the queue with kernel mode, and begin the scheduling loop
        case 0x1A:
            initialize_scheduler(state->task_queue, state->pc);
            state->scheduler = true;
            break;
        // Switch to a specific task, takes argument of task's unique id. Update the task queue accordingly
        case 0x1B:
            yield_task(state->task_queue, state->reg[operand_rd]);
            break;
        // Kill a specific task, takes argument of task's unique id. Remove the task from the task queue and free the memory allocated for the task.
        case 0x1C:
            kill_task(state->task_queue, state->reg[operand_rd]);
            break;
        // SIGILL
        default:
            printf("SIGILL: at state of program counter: %d\n", *state->pc);
            printf("Instruction: %x was called\n", opcode);
            return true;
    }
    increment_pc(state, opcode);
    return false;
}