#include "main.h"
#include <stdint.h>
#include <stdlib.h>


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

    if(*queue->size > 0) {
        for (uint8_t i = *queue->size; i > 0; i--) {
            queue->sources[i] = queue->sources[i - 1];
        }
    }
    queue->sources[0] = source;
    (*queue->size)++;

    printf("Size: %d Queue: ", *queue->size);

    for (int i = 0; i < *queue->size; i++) {
        printf("%02x ", queue->sources[i]);
    }
    printf("\n");
}

uint8_t pop_interrupt(InterruptQueue* queue) {
    printf("popping interrupt, q size %d\n", *queue->size);
    if (queue->size == 0) {
        return 0;
    }
    uint8_t source = queue->sources[*queue->size - 1];

    (*queue->size)--;

    printf("popped interrupt, q size %d\n", *queue->size);
    return source;
}
