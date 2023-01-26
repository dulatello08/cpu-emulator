#include "main.h"
#include <stdint.h>

void push_task(TaskQueue *task_queue, Task *task) {
    if (task_queue->size < TASK_PARALLEL) {
        for (int i = task_queue->size; i > 0; i--) {
            task_queue->tasks[i] = task_queue->tasks[i - 1];
        }
        memcpy(&task_queue->tasks[0], task, sizeof(Task));
        task_queue->size++;
    }
}

void initialize_scheduler(TaskQueue *task_queue, uint8_t *program_counter){
    Task *kernel_task = calloc(1, sizeof(Task));
    kernel_task->pid = 0;
    kernel_task->priority = 10;
    kernel_task->program_counter = program_counter;
    task_queue->tasks = calloc(TASK_PARALLEL, sizeof(Task));
    push_task(task_queue, kernel_task);
    free(kernel_task);
}

uint8_t create_task(TaskQueue *task_queue, uint8_t *data_memory, uint8_t entry_point) {
    // Find the next available PID
    uint8_t pid = task_queue->tasks[task_queue->size]->pid + 1;

    // Allocate memory for the new task
    Task *new_task = calloc(1, sizeof(Task));
    new_task->pid = pid;
    new_task->priority = 1; // Default priority
    memcpy(new_task->program_counter, &entry_point, sizeof(uint8_t));
    create_uint16_array((const uint8_t **) &data_memory, new_task->program_memory, entry_point, DATA_MEMORY-entry_point);

    // Insert the task into the task queue
    push_task(task_queue, new_task);
    free(new_task);
    return pid;
}

int cmp_tasks_by_priority(const void *a, const void *b) {
    const Task *task1 = (Task *)a;
    const Task *task2 = (Task *)b;
    return task1->priority - task2->priority;
}

void schedule(CPUState *state, uint8_t *flash_memory) {
    TaskQueue *task_queue = state->task_queue;
    /*
    Find the next highest priority task that is ready to run
    calculate time slice for each task based on their priority
    */
    int time_slice[task_queue->size];
    int total_priority = 0;
    for (int i = 0; i < task_queue->size; i++) {
        total_priority += task_queue->tasks[i]->priority;
    }
    for (int i = 0; i < task_queue->size; i++) {
        time_slice[i] = (task_queue->tasks[i]->priority / total_priority) * TIME_SLOT;
    }
    for (int i = 0; i < task_queue->size; i++) {
        task_queue->tasks[i]->time_slice = time_slice[i];
    }
    /*
    Sort the task queue by task priority
    */
    qsort(task_queue->tasks, task_queue->size, sizeof(Task), cmp_tasks_by_priority);
    /*
    Increase the time running of the current task
    */
    while(task_queue->size>0) {
        execute_sch_instruction(state, task_queue->tasks[task_queue->head]->program_counter, task_queue->tasks[task_queue->head]->program_memory, flash_memory);
        task_queue->tasks[task_queue->head]->time_running++;
        /*
        Check if the current task has exceeded its time slice
        */
        if (task_queue->tasks[task_queue->head]->time_running >= task_queue->tasks[task_queue->head]->time_slice) {
            /*
            Move the current task to the end of the queue
            */
            Task *current_task = task_queue->tasks[task_queue->head];
            for (int i = task_queue->head; i < task_queue->size - 1; i++) {
                task_queue->tasks[i] = task_queue->tasks[i + 1];
            }
            current_task->time_running = 0;
            task_queue->tasks[task_queue->size - 1] = current_task;
            /*
            Set the head of the task queue to the next task
            */
            task_queue->tasks[task_queue->head]->time_running++;
            if (task_queue->tasks[task_queue->head]->time_running >= task_queue->tasks[task_queue->head]->time_slice) {
                current_task = task_queue->tasks[task_queue->head];
                for (int i = task_queue->head; i < task_queue->size - 1; i++) {
                    task_queue->tasks[i] = task_queue->tasks[i + 1];
                }
                current_task->time_running = 0;
                task_queue->tasks[task_queue->size - 1] = current_task;
                task_queue->head = 0;
            }
        }
    }
}

void yield_task(TaskQueue *task_queue, uint8_t pid) {
    // Find the task with the given pid in the task queue
    int task_index = -1;
    for (int i = 0; i < task_queue->size; i++) {
        if (task_queue->tasks[i]->pid == pid) {
            task_index = i;
            break;
        }
    }
    if (task_index == -1) {
        fprintf(stderr, "Error: Task with pid %d not found in task queue", pid);
        return;
    }
    // Move the task with the given pid to the front of the queue
    Task *task_to_yield = task_queue->tasks[task_index];
    for (int i = task_index; i > 0; i--) {
        task_queue->tasks[i] = task_queue->tasks[i - 1];
    }
    task_to_yield->time_running = 0;
    task_queue->tasks[0] = task_to_yield;
    task_queue->head = 0;
}

void kill_task(TaskQueue *task_queue, uint8_t pid) {
    int task_index = -1;
    for (int i = 0; i < task_queue->size; i++) {
        if (task_queue->tasks[i]->pid == pid) {
            task_index = i;
            break;
        }
    }
    if (task_index == -1) {
        fprintf(stderr, "Error: Task with PID %d not found.\n", pid);
        return;
    }
    for (int i = task_index; i < task_queue->size - 1; i++) {
        task_queue->tasks[i] = task_queue->tasks[i + 1];
    }
    task_queue->size--;
    if (task_index < task_queue->head) {
        task_queue->head--;
    }
    printf("Task with PID %d has been killed.\n", pid);
}