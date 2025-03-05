#include "main.h"
#include <stdint.h>

// ================================
// Interrupt Vector Table Functions
// ================================

// Initialize the interrupt vector table
InterruptVectorTable* init_interrupt_vector_table(void) {
    InterruptVectorTable *table = malloc(sizeof(InterruptVectorTable));
    if (!table) {
        perror("Failed to allocate InterruptVectorTable");
        exit(EXIT_FAILURE);
    }
    table->count = 0;
    for (int i = 0; i < MAX_INTERRUPTS; i++) {
        table->entries[i].source = 0;
        table->entries[i].handler_address = 0;
    }
    return table;
}

// Register a new interrupt vector (returns false if table is full)
bool register_interrupt_vector(InterruptVectorTable *table, uint8_t source, uint32_t handler_address) {
    if (table->count >= MAX_INTERRUPTS)
        return false;

    // Optionally, check if the interrupt is already registered (skipped here)
    table->entries[table->count].source = source;
    table->entries[table->count].handler_address = handler_address;
    table->count++;
    return true;
}

// Unregister an interrupt vector by source (returns false if not found)
bool unregister_interrupt_vector(InterruptVectorTable *table, uint8_t source) {
    for (int i = 0; i < table->count; i++) {
        if (table->entries[i].source == source) {
            // Shift remaining entries left to fill the gap
            for (int j = i; j < table->count - 1; j++) {
                table->entries[j] = table->entries[j + 1];
            }
            table->count--;
            return true;
        }
    }
    return false;
}

// Retrieve an interrupt vector entry by source (returns NULL if not found)
InterruptVectorEntry* get_interrupt_vector(InterruptVectorTable *table, uint8_t source) {
    for (int i = 0; i < table->count; i++) {
        if (table->entries[i].source == source) {
            return &table->entries[i];
        }
    }
    return NULL;
}

// ========================
// Interrupt Queue Functions
// ========================

// Initialize the interrupt FIFO queue
InterruptQueue* init_interrupt_queue(void) {
    InterruptQueue *queue = malloc(sizeof(InterruptQueue));
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
    return queue;
}

// Enqueue an interrupt into the queue (returns false if the queue is full)
bool enqueue_interrupt(InterruptQueue *queue, uint8_t irq) {
    pthread_mutex_lock(&queue->mutex);
    if (queue->count >= IRQ_QUEUE_SIZE) {
        pthread_mutex_unlock(&queue->mutex);
        // Queue is full
        return false;
    }
    queue->queue[queue->tail] = irq;
    queue->tail = (queue->tail + 1) % IRQ_QUEUE_SIZE;
    queue->count++;
    // Signal any thread waiting in the WFI instruction
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
    return true;
}

// Dequeue an interrupt from the queue (returns false if the queue is empty)
bool dequeue_interrupt(InterruptQueue *queue, uint8_t *irq) {
    pthread_mutex_lock(&queue->mutex);
    if (queue->count == 0) {
        pthread_mutex_unlock(&queue->mutex);
        // Queue is empty
        return false;
    }
    *irq = queue->queue[queue->head];
    queue->head = (queue->head + 1) % IRQ_QUEUE_SIZE;
    queue->count--;
    pthread_mutex_unlock(&queue->mutex);
    return true;
}

// Check if the interrupt queue is empty
bool is_interrupt_queue_empty(InterruptQueue *queue) {
    pthread_mutex_lock(&queue->mutex);
    bool empty = (queue->count == 0);
    pthread_mutex_unlock(&queue->mutex);
    return empty;
}
// Check if the interrupt queue is full
bool is_interrupt_queue_full(InterruptQueue *queue) {
    pthread_mutex_lock(&queue->mutex);
    bool full = (queue->count >= IRQ_QUEUE_SIZE);
    pthread_mutex_unlock(&queue->mutex);
    return full;
}
