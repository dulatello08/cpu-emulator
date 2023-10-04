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
    (*queue->size)++;

    size_t new_size = *queue->size * sizeof(uint8_t);

    void *new_sources = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (new_sources == MAP_FAILED) {
        perror("mmap");
        return;  // Handle the error appropriately
    }

    // Copy data from the old memory block to the new one
    memcpy(new_sources, queue->sources, (*queue->size - 1) * sizeof(uint8_t));

    if (queue->sources != MAP_FAILED) {
        munmap(queue->sources, (*queue->size - 1) * sizeof(uint8_t));
    }

    queue->sources = new_sources;
    for (uint8_t i = *queue->size - 1; i > 0; i--) {
        queue->sources[i] = queue->sources[i - 1];
    }
    queue->sources[0] = source;
    printf("Size: %d Queue: ", *queue->size);

    for (int i = 0; i < *queue->size; i++) {
        printf("%02x ", queue->sources[i]);
    }
    printf("\n");
}

uint8_t pop_interrupt(InterruptQueue* queue) {

    if (*queue->size == 0) {
        return 0;
    }
    printf("q size %d\n", *queue->size);
    uint8_t source = queue->sources[*queue->size - 1];

    queue->size--;
    // Unmap the old memory block
    if(munmap(&(queue->sources[*queue->size]), 1) == -1) {
        perror("mmap");
        return 0;
    }

    return source;
}
