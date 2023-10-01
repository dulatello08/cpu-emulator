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
    queue->size++;

    size_t new_size = queue->size * sizeof(uint8_t);

    void *new_sources = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (new_sources == MAP_FAILED) {
        perror("mmap");
        return;  // Handle the error appropriately
    }

    // Copy data from the old memory block to the new one
    memcpy(new_sources, queue->sources, (queue->size - 1) * sizeof(uint8_t));

    if (queue->sources != MAP_FAILED) {
        munmap(queue->sources, (queue->size - 1) * sizeof(uint8_t));
    }

    queue->sources = new_sources;
    queue->sources[queue->size - 1] = source;
    printf("Size: %d Queue: ", queue->size);

    for (int i = 0; i < queue->size; i++) {
        printf("%02x ", queue->sources[i]);
    }
}

uint8_t pop_interrupt(InterruptQueue* queue) {
    if (queue->size == 0) {
        return 0;
    }

    uint8_t source = queue->sources[queue->size - 1];

    queue->size--;

    if (queue->size == 0) {
        // If the queue is empty, simply unmap the old memory block
        munmap(queue->sources, 0);
        queue->sources = NULL;
    } else {
        // Allocate a new memory block with the reduced size
        size_t new_size = queue->size * sizeof(uint8_t);
        void *new_sources = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

        if (new_sources == MAP_FAILED) {
            perror("mmap");
            return 0;  // Handle the error appropriately
        }

        // Copy data from the old memory block to the new one (excluding the last element)
        memcpy(new_sources, queue->sources, new_size);

        // Unmap the old memory block
        munmap(queue->sources, queue->size * sizeof(uint8_t));

        queue->sources = new_sources;
    }

    return source;
}
