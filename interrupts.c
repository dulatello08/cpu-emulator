#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// Define a structure for the interrupt queue
struct interrupt_queue {
    uint8_t* elements; // Dynamically allocated array to store interrupt sources
    int top; // Index of the top element
};

typedef struct interrupt_queue InterruptQueue; // Typedef for convenience

// Initialize the interrupt queue with a specified capacity
InterruptQueue* create_interrupt_queue(int capacity) {
    InterruptQueue* queue = (InterruptQueue*)malloc(sizeof(InterruptQueue));
    if (queue == NULL) {
        return NULL; // Allocation failed
    }

    queue->elements = (uint8_t*)calloc(capacity, sizeof(uint8_t));
    if (queue->elements == NULL) {
        free(queue);
        return NULL; // Allocation failed
    }

    queue->top = -1;
    return queue;
}

// Check if the interrupt queue is empty
bool is_interrupt_queue_empty(const InterruptQueue* queue) {
    return queue->top == -1;
}


// Push an interrupt onto the queue
bool push_interrupt(InterruptQueue* queue, uint8_t source) {
    queue->elements[++queue->top] = source;
    return true;
}

// Pop an interrupt from the queue
bool pop_interrupt(InterruptQueue* queue, uint8_t* source) {
    if (is_interrupt_queue_empty(queue)) {
        return false; // Queue is empty
    }

    *source = queue->elements[queue->top--];
    return true;
}
