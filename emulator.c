#include "main.h"
#include <stdint.h>

uint8_t count_leading_zeros(uint8_t x) {
    uint8_t count = 0;

    while (x != 0) {
        x >>= 1;
        count++;
    }

    return 8 - count;
}

void push(ShiftStack* stack, uint8_t value) {
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

uint8_t pop(ShiftStack* stack) {
    if (stack->top == -1) {
        // Stack is empty, return 0
        return 0;
    }
    uint8_t value = stack->data[stack->top];
    stack->top--;
    return value;
}

int start(size_t program_size, size_t flash_size, const uint8_t* program_memory, uint8_t* flash_memory, uint8_t* memory) {
    CPUState state = {
            .ssr = {.top = -1},
    };
    state.pc = calloc(1, sizeof(uint8_t));
    state.reg = calloc(16, sizeof(uint8_t));
    state.v_flag = false;
    state.z_flag = false;
    state.scheduler = false;
    state.memory = memory;
    memcpy(&(state.memory), program_memory, program_size);

    printf("Memory map for HI: \n");
    printf("0x0000 - %04x : Program memory\n", (int) program_size);
    size_t usableMemorySize = UINT16_MAX - program_size - 4096 - 1;
    printf("%04x - %04x : Usable memory\n", (int) program_size, (int) program_size + (int) usableMemorySize);
    printf("%04x : Flash IO Port\n", (int) program_size + (int) usableMemorySize);
    printf("%04x - 0xFFFF : Current flash block\n", (int) program_size + (int) usableMemorySize + 1);

    memcpy(&(state.memory[(int) program_size + (int) usableMemorySize + 1]), flash_memory, 4096);

    state.task_queue = calloc(1, sizeof(TaskQueue));
    state.task_queue->tasks = calloc(1, sizeof(Task*));
    state.task_queue->tasks[0] = calloc(TASK_PARALLEL, sizeof(Task));
    printf("Starting emulator\n");
    bool exitCode = false;
    while (*(state.pc) + 1 < EXPECTED_PROGRAM_WORDS && !exitCode) {
        if (!state.scheduler) {
            exitCode = execute_instruction(&state);
        } else {
            printf("Entering scheduling loop\n");
            break;
        }
    }
    if (state.scheduler) {
       schedule(&state);
    }
    return 0;
}