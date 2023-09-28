#include "main.h"


void add_interrupt_vector(InterruptVectors table[INTERRUPT_TABLE_SIZE], uint8_t index, uint8_t source, uint16_t handler) {
    if (index < INTERRUPT_TABLE_SIZE) {
        table[index].source = source;
        table[index].handler = handler;
    }
}

uint16_t get_interrupt_handler(const InterruptVectors table[INTERRUPT_TABLE_SIZE], uint8_t source) {
    for (int i = 0; i < INTERRUPT_TABLE_SIZE; i++) {
        if (table[i].source == source) {
            return table[i].handler;
        }
    }
    // Return a default value or signal that the source was not found.
    return 0; // You can use another suitable default value.
}

void push_interrupt(InterruptQueue* queue, uint8_t source) {
    if (queue->top == INTERRUPT_QUEUE_MAX - 1) {
        // Queue is already full
        return;
    }

    // Increment the top index
    queue->top++;

    // Reallocate memory for the sources array
    queue->sources = realloc(queue->sources, (queue->top + 1) * sizeof(uint8_t));

    // Shift all elements to the right
    for (uint8_t i = queue->top; i > 0; i--) {
        queue->sources[i] = queue->sources[i - 1];
    }

    // Add the new source to the front of the queue
    queue->sources[0] = source;
}

uint8_t pop_interrupt(InterruptQueue* queue) {
    if (queue->top == SENTINEL_VALUE) {
        // Queue is empty
        return 0; // Or any other appropriate value
    }

    // Get the front source
    uint8_t source = queue->sources[0];

    // Shift all elements to the left
    for (uint8_t i = 0; i < queue->top; i++) {
        queue->sources[i] = queue->sources[i + 1];
    }

    // Decrement the top index
    queue->top--;

    // Reallocate memory for the sources array
    queue->sources = realloc(queue->sources, (queue->top + 1) * sizeof(uint8_t));

    return source;
}