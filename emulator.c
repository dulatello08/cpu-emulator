#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "main.h"
#include <string.h>

#define DATA_MEMORY 256
#define STACK_SIZE 4



typedef struct {
    uint8_t data[STACK_SIZE];
    int top;
} ShiftStack;


struct CPUState {
    // Program counter
    volatile uint8_t pc;

    // General-purpose registers
    volatile uint8_t reg[2];

    // Memory
    volatile uint16_t *program_memory;
    volatile uint8_t *data_memory;
    volatile uint8_t *flash_memory;

    // Stack shift register
    volatile ShiftStack ssr;

    // ALU Flags register
    volatile bool z_flag;
    volatile bool v_flag;
    volatile bool ascii_flag;
};

uint8_t count_leading_zeros(uint8_t x) {
    uint8_t count = 0;

    while (x != 0) {
        x >>= 1;
        count++;
    }

    return 8 - count;
}

void push(volatile ShiftStack *stack, uint8_t value) {
    if (stack->top == STACK_SIZE - 1) {
        // Shift all values in the stack down one position
        for (int i = 0; i < STACK_SIZE - 1; i++) {
            stack->data[i] = stack->data[i + 1];
        }
    } else {
        stack->top++;
    }
    stack->data[stack->top] = value;
}

uint8_t pop(volatile ShiftStack *stack) {
    if (stack->top == -1) {
        // Stack is empty, return 0
        return 0;
    }
    uint8_t value = stack->data[stack->top];
    stack->top--;
    return value;
}

int start(uint16_t *program_memory) {
    struct CPUState state = {
            .ssr = {.top = -1},
    };
    state.pc = 0x00;
    state.reg[0] = 0x00;
    state.reg[1] = 0x00;
    state.v_flag = false;
    state.z_flag = false;

    state.flash_memory = calloc(DATA_MEMORY, sizeof(uint8_t));
    state.data_memory = calloc(DATA_MEMORY, sizeof(uint8_t));
    state.program_memory = program_memory;
    if (state.program_memory == NULL || state.data_memory == NULL) {
        // Handle allocation failure
        return 1;
    }
    printf("Starting emulator\n");
    while(state.pc!=0xFF) {
        uint8_t opcode = state.program_memory[state.pc] & 0x6F;
        bool operand_rd = state.program_memory[state.pc] & 0x70;
        bool operand_rn = state.program_memory[state.pc] & 0x80;
        uint8_t operand2 = state.program_memory[state.pc] >> 8;
        printf("Program counter: %d\n", state.pc);
        switch(opcode) {
            // Do nothing
            case 0x00:
                break;
            // Add operand 2 to the value in the operand Rd
            case 0x01:
                if (state.reg[operand_rd] > UINT8_MAX - operand2) {
                    state.v_flag = true;
                    state.reg[operand_rd] = 0xFF;
                } else {
                    state.v_flag = false;
                    state.reg[operand_rd] += operand2;
                    if (state.reg[operand_rd] == 0) {
                        state.z_flag = true;
                    }
                    else {
                        state.z_flag = false;
                    }
                }
                break;
            // Subtract operand 2 from the value in the operand Rd
            case 0x02:
                if (state.reg[operand_rd] < operand2) {
                    state.v_flag = true;
                } else {
                    state.v_flag = false;
                    state.reg[operand_rd] -= operand2;
                    if (state.reg[operand_rd] == 0) {
                        state.z_flag = true;
                    }
                    else {
                        state.z_flag = false;
                    }
                }
                break;
            // Multiply the value in the operand Rd by operand 2
            case 0x03:
                if(state.reg[operand_rd] * operand2 > UINT8_MAX) {
                    state.v_flag = true;
                    state.reg[operand_rd] = 0xFF;
                } else {
                    state.v_flag = false;
                    state.reg[operand_rd] *= operand2;
                    if (state.reg[operand_rd] == 0) {
                        state.z_flag = true;
                    }
                    else {
                        state.z_flag = false;
                    }
                }
                break;
            // Store sum of memory address at operand 2 and register Rn in register Rd
            case 0x04:
                state.reg[operand_rd] = count_leading_zeros(operand2);
                if (state.reg[operand_rd] == 0) {
                    state.z_flag = true;
                }
                else {
                    state.z_flag = false;
                }
                break;
            // Store difference of memory address at operand2 and register Rn in register Rd
            case 0x05:
                if (state.reg[operand_rd] > UINT8_MAX - state.data_memory[operand2]) {
                    state.v_flag = true;
                    state.reg[operand_rd] = 0xFF;
                } else {
                    state.v_flag = false;
                    state.reg[operand_rd] += state.data_memory[operand2];
                    if (state.reg[operand_rd] == 0) {
                        state.z_flag = true;
                    }
                    else {
                        state.z_flag = false;
                    }
                }
                break;
            // Multiply register Rn by memory address at operand 2 and store in register Rd
            case 0x06:
                if (state.reg[operand_rd] < state.data_memory[operand2]) {
                    state.v_flag = true;
                } else {
                    state.v_flag = false;
                    state.reg[operand_rd] -= state.data_memory[operand2];
                    if (state.reg[operand_rd] == 0) {
                        state.z_flag = true;
                    }
                    else {
                        state.z_flag = false;
                    }
                }
                break;
            // Store sum of registers Rd and Rn in memory address at operand 2
            case 0x07:
                state.reg[operand_rd] = operand2;
                state.ascii_flag = true;
                break;

            // Store sum of registers Rd and Rn in memory address at operand 2
            case 0x08:
                if (state.data_memory[operand2] > UINT8_MAX - state.reg[operand_rd]) {
                    state.v_flag = true;
                    state.data_memory[operand2] = 0xFF;
                } else {
                    state.v_flag = false;
                    state.data_memory[operand2] += state.reg[operand_rd];
                    if (state.data_memory[operand2] == 0) {
                        state.z_flag = true;
                    }
                    else {
                        state.z_flag = false;
                    }
                }
                break;
            // Multiply registers Rd and Rn and store in memory address at operand 2
            case 0x09:
                if (state.data_memory[operand2] < state.reg[operand_rd]) {
                    state.v_flag = true;
                } else {
                    state.v_flag = false;
                    state.data_memory[operand2] -= state.reg[operand_rd];
                    if (state.data_memory[operand2] == 0) {
                        state.z_flag = true;
                    }
                    else {
                        state.z_flag = false;
                    }
                }
                break;
            // Count the number of leading zeros at register Rn and store at Rd
            case 0x0A:
                if(state.reg[operand_rd] * state.data_memory[operand2] > UINT8_MAX) {
                    state.v_flag = true;
                    state.reg[operand_rd] = 0xFF;
                } else {
                    state.v_flag = false;
                    state.reg[operand_rd] *= state.data_memory[operand2];
                    if (state.reg[operand_rd] == 0) {
                        state.z_flag = true;
                    }
                    else {
                        state.z_flag = false;
                    }
                }
                break;         
            // Store operand 2 in the operand Rd
            case 0x0B:
                if(state.data_memory[operand2] * state.reg[operand_rd] > UINT8_MAX) {
                    state.v_flag = true;
                    state.data_memory[operand2] = 0xFF;
                } else {
                    state.v_flag = false;
                    state.data_memory[operand2] *= state.reg[operand_rd];
                    if (state.data_memory[operand2] == 0) {
                        state.z_flag = true;
                    }
                    else {
                        state.z_flag = false;
                    }
                }
                break;
            // Store the value in the register Rd in the data memory at the operand 2
            case 0x0C:
                state.reg[operand_rd] = operand2;
                break;
            // Load the value in the memory at the address in operand 2 into the register Rd
            case 0x0D:
                state.data_memory[operand2] = state.reg[operand_rd];
                break;
            // Push the value in the register Rn at the specified address onto a stack
            case 0x0E:
                state.reg[operand_rd] = state.data_memory[operand2];
                break;
            // Pop a value from the stack and store it in the register Rd
            case 0x0F:
                push(&state.ssr, state.reg[operand_rd]);
                if(state.ascii_flag){
                    char output = (char) state.reg[operand_rd];
                    printf("%c", output);
                    state.ascii_flag=false;
                } else {
                    printf("%d\n", state.reg[operand_rd]);
                }
                break;
            // Read data memory and store in non-volatile memory using addresses from registers Rd and Rn, starting at address in Operand 2
            case 0x12:
                state.data_memory[operand2] = pop(&state.ssr);
                break;
            // Branch to value specified in operand 2
            case 0x13:
                state.pc = operand2;
                break;
            // Branch to value specified in operand2 if zero flag was set
            case 0x14:
                if (state.z_flag) {
                    state.pc = operand2;
                }
                break;
            // Branch to value specified in operand2 if overflow flag was set.
            case 0x15:
                if (state.v_flag) {
                    state.pc = operand2;
                }
                break;
            // Branch to value specified in operand2 if register at operand 1 equals to opposite register
            case 0x16:
                if(state.reg[operand_rd]==state.reg[operand_rn]) {
                    state.pc=operand2;
                }
                break;
            // Halt
            case 0x17:
                printf("Halt at state of program counter: %d\n", state.pc);
                return 0;
            // SIGILL
            default:
                printf("SIGILL: at state of program counter: %d\n", state.pc);
                return 0;
        }
        state.pc++;
    }
    return 0;
}
