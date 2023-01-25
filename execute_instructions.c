//
// Created by dulat on 1/24/23.
//

#include "main.h"

bool execute_sch_instruction(CPUState *state, uint8_t *program_counter, const uint16_t *program_memory, uint8_t *flash_memory) {
    uint8_t opcode = program_memory[*program_counter] & 0x3F;
    bool operand_rd = (program_memory[*program_counter] >> 6) & 0x01;
    bool operand_rn = (program_memory[*program_counter] >> 7) & 0x01;
    uint8_t operand2 = (program_memory[*program_counter] >> 8) & 0xFF;
    switch(opcode) {
        // Do nothing
        case 0x00:
            break;
        // Add operand 2 to the value in the operand Rd
        case 0x01:
            if (state->reg[operand_rd] + operand2 > UINT8_MAX) {
                state->v_flag = true;
                state->reg[operand_rd] = 0xFF;
            } else {
                state->v_flag = false;
                state->reg[operand_rd] += operand2;
                if (state->reg[operand_rd] == 0) {
                    state->z_flag = true;
                }
                else {
                    state->z_flag = false;
                }
            }
            break;
        // Subtract operand 2 from the value in the operand Rd
        case 0x02:
            if (state->reg[operand_rd] < operand2) {
                state->v_flag = true;
            } else {
                state->v_flag = false;
                state->reg[operand_rd] -= operand2;
                if (state->reg[operand_rd] == 0) {
                    state->z_flag = true;
                }
                else {
                    state->z_flag = false;
                }
            }
            break;
        // Multiply the value in the operand Rd by operand 2
        case 0x03:
            if (state->reg[operand_rd] * operand2 > UINT8_MAX) {
                state->v_flag = true;
            } else {
                state->v_flag = false;
                state->reg[operand_rd] *= operand2;
                if (state->reg[operand_rd] == 0) {
                    state->z_flag = true;
                }
                else {
                    state->z_flag = false;
                }
            }
            break;
        // Store sum of memory address at operand 2 and register Rn in register Rd
        case 0x04:
            if (state->data_memory[operand2] + state->reg[operand_rn] > UINT8_MAX) {
                state->v_flag = true;
            } else {
                state->v_flag = false;
                state->reg[operand_rd] = state->data_memory[operand2] + state->reg[operand_rn];
                if (state->reg[operand_rd] == 0) {
                    state->z_flag = true;
                }
                else {
                    state->z_flag = false;
                }
            }
            break;
        // Store difference of memory address at operand2 and register Rn in register Rd
        case 0x05:
            if (state->reg[operand_rd] > UINT8_MAX - state->reg[operand_rn]) {
                state->v_flag = true;
            } else {
                state->v_flag = false;
                state->reg[operand_rd] = state->data_memory[operand2] - state->reg[operand_rn];
                if (state->reg[operand_rd] == 0) {
                    state->z_flag = true;
                }
                else {
                    state->z_flag = false;
                }
            }
            break;
        // Multiply register Rn by memory address at operand 2 and store in register Rd
        case 0x06:
            if (state->data_memory[operand2] * state->reg[operand_rn] > UINT8_MAX) {
                state->v_flag = true;
            } else {
                state->v_flag = false;
                state->reg[operand_rd] = state->data_memory[operand2] * state->reg[operand_rn];
                if (state->reg[operand_rd] == 0) {
                    state->z_flag = true;
                }
                else {
                    state->z_flag = false;
                }
            }
            break;
        // Store sum of registers Rd and Rn in memory address at operand 2
        case 0x07:
            if (state->reg[operand_rd] + state->reg[operand_rn] > UINT8_MAX) {
                state->v_flag = true;
                state->reg[operand_rd] = 0xFF;
            } else {
                state->v_flag = false;
                state->data_memory[operand2] = state->reg[operand_rd] + state->reg[operand_rn];
                if (state->data_memory[operand2] == 0) {
                    state->z_flag = true;
                }
                else {
                    state->z_flag = false;
                }
            }
            break;
        // Store difference of registers Rd and Rn in memory address at operand 2
        case 0x08:
            if (state->reg[operand_rd] > UINT8_MAX - state->reg[operand_rn]) {
                state->v_flag = true;
            } else {
                state->v_flag = false;
                state->data_memory[operand2] = state->reg[operand_rd] - state->reg[operand_rn];
                if (state->data_memory[operand2] == 0) {
                    state->z_flag = true;
                }
                else {
                    state->z_flag = false;
                }
            }
            break;
        // Multiply registers Rd and Rn and store in memory address at operand 2
        case 0x09:
            if (state->reg[operand_rd] * state->reg[operand_rn] > UINT8_MAX) {
                state->v_flag = true;
            } else {
                state->v_flag = false;
                state->data_memory[operand2] = state->reg[operand_rd] * state->reg[operand_rn];
                if (state->data_memory[operand2] == 0) {
                    state->z_flag = true;
                }
                else {
                    state->z_flag = false;
                }
            }
            break;

        // Count the number of leading zeros at register Rn and store at Rd
        case 0x0A:
            state->reg[operand_rd] = count_leading_zeros(state->reg[operand_rn]);
            if (state->reg[operand_rd] == 0) {
                state->z_flag = true;
            }
            else {
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
            state->data_memory[operand2] = state->reg[operand_rd];
            break;
        // Load the value in the memory at the address in operand 2 into the register Rd
        case 0x0D:
            state->reg[operand_rd] = state->data_memory[operand2];
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
            state->data_memory[operand2] = state->data_memory[state->reg[operand_rd]];
            break;
        // Read non-volatile memory and store in data memory using addresses from operands 2 and Rn, starting at address in Rd
        case 0x11:
            {
                uint8_t* temp;
                uint8_t size = state->reg[operand_rd] - state->reg[operand_rn];
                if (size > 0) {
                    temp = (uint8_t*) calloc(size, sizeof(uint8_t));
                    memcpy(temp, (void*)&flash_memory[state->reg[operand_rn]], size);
                    memcpy((void*)&state->data_memory[operand2], temp, size);
                    free(temp);
                }
            }
            break;
        // Read data memory and store in non-volatile memory using addresses from registers Rd and Rn, starting at address in Operand 2
        case 0x12:
            {
                uint8_t* temp;
                uint8_t size = state->reg[operand_rd] - state->reg[operand_rn];
                if (size > 0) {
                    temp = (uint8_t *) calloc(size, sizeof(uint8_t));
                    memcpy(temp, (void *) &state->data_memory[state->reg[operand_rn]], size);
                    memcpy((void *) &flash_memory[operand2], temp, size);
                    free(temp);
                }
            }
            break;
        // Branch to value specified in operand 2
        case 0x13:
            *program_counter = operand2;
            break;
        // Branch to value specified in operand2 if zero flag was set
        case 0x14:
            if (state->z_flag) {
                *program_counter = operand2;
            }
            break;
        // Branch to value specified in operand2 if overflow flag was not set.
        case 0x15:
            if (!state->v_flag) {
                *program_counter = operand2;
            }
            break;
        // Branch to value specified in operand2 if register at operand 1 equals to opposite register
        case 0x16:
            if (state->reg[operand_rd]==state->reg[operand_rn]) {
                *program_counter = operand2;
            }
            break;
        // Branch to value specified in operand2 if register at operand 1 does not equal to opposite register
        case 0x17:
            if (state->reg[operand_rd]!=state->reg[operand_rn]) {
                *program_counter = operand2;
            }
            break;
        // Halt
        case 0x18:
            if (state->cpu_mode) {
                printf("Halt in unprivileged task");
                break;
            }
            printf("Halt at state of program counter: %d\n", *program_counter);
            return true;
        // Create a new task, takes argument of memory address of the task's entry point. Insert the task into the task queue.
        case 0x19:
            state->reg[operand_rd] = create_task(state->task_queue, operand2);
            break;
        // Start the scheduler, should initialize the task queue, set the current task to the first task in the queue with kernel mode, and begin the scheduling loop
        case 0x1A:
            initialize_scheduler(state->task_queue, &state->pc);
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
            printf("SIGILL: at state of program counter: %d\n", *program_counter);
            printf("Instruction: %x was called\n", opcode);
            return true;
    }
    return false;
}

bool execute_instruction(CPUState *state, uint8_t *flash_memory) {
    uint8_t opcode = state->program_memory[state->pc] & 0x3F;
    bool operand_rd = (state->program_memory[state->pc] >> 6) & 0x01;
    bool operand_rn = (state->program_memory[state->pc] >> 7) & 0x01;
    uint8_t operand2 = (state->program_memory[state->pc] >> 8) & 0xFF;
    switch(opcode) {
        // Do nothing
        case 0x00:
            break;
        // Add operand 2 to the value in the operand Rd
        case 0x01:
            if (state->reg[operand_rd] + operand2 > UINT8_MAX) {
                state->v_flag = true;
                state->reg[operand_rd] = 0xFF;
            } else {
                state->v_flag = false;
                state->reg[operand_rd] += operand2;
                if (state->reg[operand_rd] == 0) {
                    state->z_flag = true;
                }
                else {
                    state->z_flag = false;
                }
            }
            break;
        // Subtract operand 2 from the value in the operand Rd
        case 0x02:
            if (state->reg[operand_rd] < operand2) {
                state->v_flag = true;
            } else {
                state->v_flag = false;
                state->reg[operand_rd] -= operand2;
                if (state->reg[operand_rd] == 0) {
                    state->z_flag = true;
                }
                else {
                    state->z_flag = false;
                }
            }
            break;
        // Multiply the value in the operand Rd by operand 2
        case 0x03:
            if (state->reg[operand_rd] * operand2 > UINT8_MAX) {
                state->v_flag = true;
            } else {
                state->v_flag = false;
                state->reg[operand_rd] *= operand2;
                if (state->reg[operand_rd] == 0) {
                    state->z_flag = true;
                }
                else {
                    state->z_flag = false;
                }
            }
            break;
        // Store sum of memory address at operand 2 and register Rn in register Rd
        case 0x04:
            if (state->data_memory[operand2] + state->reg[operand_rn] > UINT8_MAX) {
                state->v_flag = true;
            } else {
                state->v_flag = false;
                state->reg[operand_rd] = state->data_memory[operand2] + state->reg[operand_rn];
                if (state->reg[operand_rd] == 0) {
                    state->z_flag = true;
                }
                else {
                    state->z_flag = false;
                }
            }
            break;
        // Store difference of memory address at operand2 and register Rn in register Rd
        case 0x05:
            if (state->reg[operand_rd] > UINT8_MAX - state->reg[operand_rn]) {
                state->v_flag = true;
            } else {
                state->v_flag = false;
                state->reg[operand_rd] = state->data_memory[operand2] - state->reg[operand_rn];
                if (state->reg[operand_rd] == 0) {
                    state->z_flag = true;
                }
                else {
                    state->z_flag = false;
                }
            }
            break;
        // Multiply register Rn by memory address at operand 2 and store in register Rd
        case 0x06:
            if (state->data_memory[operand2] * state->reg[operand_rn] > UINT8_MAX) {
                state->v_flag = true;
            } else {
                state->v_flag = false;
                state->reg[operand_rd] = state->data_memory[operand2] * state->reg[operand_rn];
                if (state->reg[operand_rd] == 0) {
                    state->z_flag = true;
                }
                else {
                    state->z_flag = false;
                }
            }
            break;
        // Store sum of registers Rd and Rn in memory address at operand 2
        case 0x07:
            if (state->reg[operand_rd] + state->reg[operand_rn] > UINT8_MAX) {
                state->v_flag = true;
                state->reg[operand_rd] = 0xFF;
            } else {
                state->v_flag = false;
                state->data_memory[operand2] = state->reg[operand_rd] + state->reg[operand_rn];
                if (state->data_memory[operand2] == 0) {
                    state->z_flag = true;
                }
                else {
                    state->z_flag = false;
                }
            }
            break;
        // Store difference of registers Rd and Rn in memory address at operand 2
        case 0x08:
            if (state->reg[operand_rd] > UINT8_MAX - state->reg[operand_rn]) {
                state->v_flag = true;
            } else {
                state->v_flag = false;
                state->data_memory[operand2] = state->reg[operand_rd] - state->reg[operand_rn];
                if (state->data_memory[operand2] == 0) {
                    state->z_flag = true;
                }
                else {
                    state->z_flag = false;
                }
            }
            break;
        // Multiply registers Rd and Rn and store in memory address at operand 2
        case 0x09:
            if (state->reg[operand_rd] * state->reg[operand_rn] > UINT8_MAX) {
                state->v_flag = true;
            } else {
                state->v_flag = false;
                state->data_memory[operand2] = state->reg[operand_rd] * state->reg[operand_rn];
                if (state->data_memory[operand2] == 0) {
                    state->z_flag = true;
                }
                else {
                    state->z_flag = false;
                }
            }
            break;

        // Count the number of leading zeros at register Rn and store at Rd
        case 0x0A:
            state->reg[operand_rd] = count_leading_zeros(state->reg[operand_rn]);
            if (state->reg[operand_rd] == 0) {
                state->z_flag = true;
            }
            else {
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
            state->data_memory[operand2] = state->reg[operand_rd];
            break;
        // Load the value in the memory at the address in operand 2 into the register Rd
        case 0x0D:
            state->reg[operand_rd] = state->data_memory[operand2];
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
            state->data_memory[operand2] = state->data_memory[state->reg[operand_rd]];
            break;
        // Read non-volatile memory and store in data memory using addresses from operands 2 and Rn, starting at address in Rd
        case 0x11:
        {
            uint8_t* temp;
            uint8_t size = state->reg[operand_rd] - state->reg[operand_rn];
            if (size > 0) {
                temp = (uint8_t*) calloc(size, sizeof(uint8_t));
                memcpy(temp, (void*)&flash_memory[state->reg[operand_rn]], size);
                memcpy((void*)&state->data_memory[operand2], temp, size);
                free(temp);
            }
        }
            break;
        // Read data memory and store in non-volatile memory using addresses from registers Rd and Rn, starting at address in Operand 2
        case 0x12:
        {
            uint8_t* temp;
            uint8_t size = state->reg[operand_rd] - state->reg[operand_rn];
            if (size > 0) {
                temp = (uint8_t *) calloc(size, sizeof(uint8_t));
                memcpy(temp, (void *) &state->data_memory[state->reg[operand_rn]], size);
                memcpy((void *) &flash_memory[operand2], temp, size);
                free(temp);
            }
        }
            break;
        // Branch to value specified in operand 2
        case 0x13:
            state->pc = operand2;
            break;
        // Branch to value specified in operand2 if zero flag was set
        case 0x14:
            if (state->z_flag) {
                state->pc = operand2;
            }
            break;
        // Branch to value specified in operand2 if overflow flag was not set.
        case 0x15:
            if (!state->v_flag) {
                state->pc = operand2;
            }
            break;
        // Branch to value specified in operand2 if register at operand 1 equals to opposite register
        case 0x16:
            if (state->reg[operand_rd]==state->reg[operand_rn]) {
                state->pc=operand2;
            }
            break;
        // Branch to value specified in operand2 if register at operand 1 does not equal to opposite register
        case 0x17:
            if (state->reg[operand_rd]!=state->reg[operand_rn]) {
                state->pc=operand2;
            }
            break;
        // Halt
        case 0x18:
            printf("Halt at state of program counter: %d\n", state->pc);
            return true;
        // Create a new task, takes argument of memory address of the task's entry point. Insert the task into the task queue.
        case 0x19:
            state->reg[operand_rd] = create_task(state->task_queue, operand2);
            break;
        // Start the scheduler, should initialize the task queue, set the current task to the first task in the queue with kernel mode, and begin the scheduling loop
        case 0x1A:
            initialize_scheduler(state->task_queue, &state->pc);
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
            printf("SIGILL: at state of program counter: %d\n", state->pc);
            printf("Instruction: %x was called\n", opcode);
            return true;
    }
    return false;
}