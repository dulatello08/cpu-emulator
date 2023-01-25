#include "main.h"

void push_task(TaskQueue *task_queue, Task *task) {
    if (task_queue->size == TASK_PARALLEL - 1) {
        // Shift all values in the stack down one position
        for (int i = 0; i < TASK_PARALLEL - 1; i++) {
            task_queue->tasks[i] = task_queue->tasks[i + 1];
        }
        memcpy(&(task_queue->tasks[TASK_PARALLEL]), task, sizeof(Task));
    }
}

void initialize_scheduler(TaskQueue *task_queue, uint8_t *program_counter){
    Task *kernel_task = calloc(1, sizeof(Task));
    kernel_task->pid = 0;
    kernel_task->priority = 10;
    kernel_task->program_counter = program_counter;
    push_task(task_queue, kernel_task);
    free(kernel_task);
}

uint8_t create_task(TaskQueue *task_queue, uint8_t entry_point) {
    // Find the next available PID
    uint8_t pid = task_queue->tasks[task_queue->size]->pid + 1;

    // Allocate memory for the new task
    Task *new_task = calloc(1, sizeof(Task));
    new_task->pid = pid;
    new_task->priority = 0; // Default priority
    memcpy(new_task->program_counter, &entry_point, sizeof(uint8_t));

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

void schedule(TaskQueue *task_queue) {
    // Find the next highest priority task that is ready to run
    // calculate time slice for each task based on their priority
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
    // sort the task queue by priority
    qsort(task_queue->tasks, task_queue->size, sizeof(Task*), cmp_tasks_by_priority);

    // find the next task to run based on their time slice
    int i = 0;
    while (i < task_queue->size && task_queue->tasks[i]->time_running > task_queue->tasks[i]->time_slice) {
        task_queue->tasks[i]->time_running -= task_queue->tasks[i]->time_slice;
        i++;
    }
    task_queue->head = i;
}