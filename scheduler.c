#include "main.h"

void push_task(TaskQueue *task_queue, Task *task) {
    if (task_queue->size == TASK_PARALLEL - 1) {
        // Shift all values in the stack down one position
        for (int i = 0; i < TASK_PARALLEL - 1; i++) {
            task_queue->tasks[i] = task_queue->tasks[i + 1];
        }
        memcpy(task_queue->tasks[TASK_PARALLEL], task, sizeof(Task));
    }
    return;
}

void initialize_scheduler(TaskQueue *task_queue, uint8_t *program_counter){
    Task *kernel_task = calloc(1, sizeof(Task));
    kernel_task->pid = 0;
    kernel_task->priority = 10;
    kernel_task->program_counter = program_counter;
    push_task(task_queue, kernel_task);
}