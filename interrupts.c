#include "main.h"

// Function to push an element onto the queue
void push(InterruptQueue* queue, uint8_t element) {
    if (queue == NULL || queue->elements == NULL || queue->top >= UINT8_MAX) {
        // Queue is not initialized correctly or is full
        return;
    }

    // Add the element and increment top
    queue->elements[queue->top++] = element;
}

// Function to pop an element from the queue (shift register behavior)
uint8_t pop(InterruptQueue* queue) {
    if (queue == NULL || queue->elements == NULL || queue->top == 0) {
        // Queue is not initialized correctly or is empty
        return 0; // Return a default value (you can choose your own)
    }

    // Get the element at the front of the queue
    uint8_t front = queue->elements[0];

    // Shift the elements to the left
    for (uint8_t i = 0; i < queue->top - 1; i++) {
        queue->elements[i] = queue->elements[i + 1];
    }

    // Decrement top to indicate one less element
    queue->top--;

    return front;
}