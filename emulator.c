#include "main.h"

uint8_t count_leading_zeros(uint8_t x) {
    uint8_t count = 0;

    while (x != 0) {
        x >>= 1;
        count++;
    }

    return 8 - count;
}

void push(ShiftStack *stack, uint8_t value) {
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

uint8_t pop(ShiftStack *stack) {
    if (stack->top == -1) {
        // Stack is empty, return 0
        return 0;
    }
    uint8_t value = stack->data[stack->top];
    stack->top--;
    return value;
}

int start(const uint8_t *program_memory, uint8_t *data_memory, uint8_t *flash_memory) {
    CPUState state = {
            .ssr = {.top = -1},
    };
    state.pc = calloc(1, sizeof(uint8_t));
    state.reg = calloc(16, sizeof(uint8_t));
    state.v_flag = false;
    state.z_flag = false;
    state.scheduler = false;
    state.data_memory = data_memory;
    state.program_memory = program_memory;
    state.task_queue = calloc(1, sizeof(TaskQueue));
    state.task_queue->tasks = calloc(1, sizeof(Task*));
    state.task_queue->tasks[0] = calloc(TASK_PARALLEL, sizeof(Task));
    state.program_memory = program_memory;
    state.flash_memory = flash_memory;
    if (state.program_memory == NULL || state.data_memory == NULL) {
        // Handle allocation failure
        return 1;
    }
    printf("Starting emulator\n");
    bool exitCode = false;
    while (*(state.pc) + 1 < EXPECTED_PROGRAM_WORDS && !exitCode) {
        if (!state.scheduler) {
            exitCode = execute_instruction(&state);
        } else {
            break;
        }
    }
    if (state.scheduler) {
       schedule(&state);
    }
    return 0;
}