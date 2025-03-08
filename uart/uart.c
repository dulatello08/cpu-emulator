//
// uart.c
// Created by Dulat S on 3/7/25.
//

#include "uart.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

// Example IRQ definitions (adjust as needed)
#define UART_IRQ_RX 1
#define UART_IRQ_TX 2

// Helper function to compute per-byte delay (in microseconds) based on baud rate.
// Assumes 10 bits per byte (1 start + 8 data + 1 stop).
static inline unsigned int compute_byte_delay(uint32_t baud_rate) {
    if (baud_rate == 0)
        baud_rate = 300;  // default to 300 baud if not set
    double seconds_per_byte = 10.0 / baud_rate;
    return (unsigned int)(seconds_per_byte * 1000000);
}

/**
 * uart_start - The UART thread function.
 * This function is intended to be started as a thread; it handles the UART's
 * state machine, processes PTY I/O, manages TX/RX buffers with proper mutex
 * locking, and simulates transmission/reception delays based on the configured
 * baud rate.
 *
 * @arg: Pointer to the UART instance.
 *
 * Returns: Always returns NULL upon thread termination.
 */
void* uart_start(void *arg) {
    UART *uart = (UART*)arg;

    // Ensure a valid baud rate; default to 300 baud if not specified.
    if (uart->config.baud_rate == 0) {
        uart->config.baud_rate = 300;
    }
    unsigned int byte_delay_us = compute_byte_delay(uart->config.baud_rate);

    while (uart->running) {
        // --- Process PTY Input: Read incoming data ---
        uint8_t buf;
        ssize_t n = read(uart->pty_master_fd, &buf, 1);
        if (n > 0) {
            // Lock RX mutex and insert the byte into the circular RX buffer.
            pthread_mutex_lock(&uart->rx_mutex);
            size_t next_head = (uart->rx_head + 1) % uart->rx_buffer_size;
            if (next_head != uart->rx_tail) {  // Check for buffer overflow.
                uart->rx_buffer[uart->rx_head] = buf;
                uart->rx_head = next_head;
                uart->status_reg |= 0x01;  // Set RX ready flag.
            } else {
                fprintf(stderr, "RX buffer overflow\n");
            }
            pthread_mutex_unlock(&uart->rx_mutex);

            // Signal that data is ready by enqueuing an RX interrupt.
            enqueue_interrupt(UART_IRQ_RX);

            // Simulate the delay for receiving one byte.
            usleep(byte_delay_us);
        }

        // --- Process TX Buffer: Write outgoing data ---
        pthread_mutex_lock(&uart->tx_mutex);
        if (uart->tx_tail != uart->tx_head) {
            // Fetch one byte from the TX circular buffer.
            uint8_t out_byte = uart->tx_buffer[uart->tx_tail];
            uart->tx_tail = (uart->tx_tail + 1) % uart->tx_buffer_size;
            pthread_mutex_unlock(&uart->tx_mutex);

            // Write the byte to the PTY.
            ssize_t written = write(uart->pty_master_fd, &out_byte, 1);
            if (written < 0) {
                perror("write to PTY failed");
            } else {
                // Set TX complete flag and signal by enqueuing an interrupt.
                uart->status_reg |= 0x02;  // For example, bit 1 indicates TX complete.
                enqueue_interrupt(UART_IRQ_TX);
            }

            // Simulate the delay for transmitting one byte.
            usleep(byte_delay_us);
        } else {
            pthread_mutex_unlock(&uart->tx_mutex);
        }

        // Sleep briefly to yield CPU time.
        usleep(1000);  // 1ms delay to prevent busy-waiting.
    }

    return NULL;
}