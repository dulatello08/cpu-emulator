//
// uart.c
// Created by Dulat S on 3/7/25.
//

#include "uart.h"
#include <pthread.h>
#include <unistd.h>
#ifdef __APPLE__
    #include <util.h>   // macOS (provides posix_openpt, grantpt, unlockpt)
#else
//hacky
#ifdef __linux__
extern int posix_openpt(int flags);
extern int grantpt(int fd);
extern int unlockpt(int fd);
#endif
#endif

#include <stdlib.h>      // Needed for posix_openpt() in some cases
#include <fcntl.h>       // File control options (O_RDWR, O_NOCTTY)
#include "main.h"

// Example IRQ definitions (adjust as needed)
#define UART_IRQ_RX 0
#define UART_IRQ_TX 1

// Helper function to compute per-byte delay (in microseconds) based on baud rate.
// Assumes 10 bits per byte (1 start + 8 data + 1 stop).
static inline unsigned int compute_byte_delay(uint32_t baud_rate) {
    if (baud_rate == 0)
        baud_rate = 115200;  // default to 115200 baud if not set
    double seconds_per_byte = 10.0 / baud_rate;
    return (unsigned int)(seconds_per_byte * 1000000);
}

// Cleanup handler: this function will be called when the thread is canceled
// or when the main loop exits normally.
static void uart_cleanup(void *arg) {
    UART *uart = (UART *)arg;

    // Close the PTY file descriptor if it's open.
    if (uart->pty_master_fd >= 0) {
        close(uart->pty_master_fd);
        uart->pty_master_fd = -1;
    }

    // Free allocated buffers.
    if (uart->tx_buffer) {
        free(uart->tx_buffer);
        uart->tx_buffer = NULL;
    }
    if (uart->rx_buffer) {
        free(uart->rx_buffer);
        uart->rx_buffer = NULL;
    }

    // Destroy mutexes.
    pthread_mutex_destroy(&uart->tx_mutex);
    pthread_mutex_destroy(&uart->rx_mutex);

    fprintf(stderr, "UART cleanup completed.\n");
}

/**
 * uart_start - The UART thread function.
 *
 * This function now takes care of initializing its own variables and resources,
 * handles the PTY file descriptor (opening it if necessary), and registers a cleanup
 * handler so that resources are freed even if the thread is cancelled.
 *
 * @arg: Pointer to the UART instance.
 *
 */
void *uart_start(void *arg) {
    UART *uart = ((AppState*)arg)->state->uart;

    // Enable thread cancellation.
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    fprintf(stderr, "UART thread started.\n");

    // Initialize internal state variables.
    uart->status_reg = 0;
    uart->tx_head = 0;
    uart->tx_tail = 0;
    uart->rx_head = 0;
    uart->rx_tail = 0;

    // Allocate buffers if not already allocated.
    if (uart->tx_buffer == NULL) {
        uart->tx_buffer = malloc(uart->tx_buffer_size);
        if (!uart->tx_buffer) {
            perror("Failed to allocate TX buffer");
            return NULL;
        }
    }
    if (uart->rx_buffer == NULL) {
        uart->rx_buffer = malloc(uart->rx_buffer_size);
        if (!uart->rx_buffer) {
            perror("Failed to allocate RX buffer");
            free(uart->tx_buffer);
            return NULL;
        }
    }

    // Initialize mutexes.
    if (pthread_mutex_init(&uart->tx_mutex, NULL) != 0) {
        perror("Failed to initialize TX mutex");
        free(uart->tx_buffer);
        free(uart->rx_buffer);
        return NULL;
    }
    if (pthread_mutex_init(&uart->rx_mutex, NULL) != 0) {
        perror("Failed to initialize RX mutex");
        pthread_mutex_destroy(&uart->tx_mutex);
        free(uart->tx_buffer);
        free(uart->rx_buffer);
        return NULL;
    }

    // Open the PTY master if not already open.
    if (uart->pty_master_fd < 0) {
        uart->pty_master_fd = posix_openpt(O_RDWR | O_NOCTTY);
        if (uart->pty_master_fd < 0) {
            perror("Failed to open PTY master");
            pthread_mutex_destroy(&uart->tx_mutex);
            pthread_mutex_destroy(&uart->rx_mutex);
            free(uart->tx_buffer);
            free(uart->rx_buffer);
            return NULL;
        }
        if (grantpt(uart->pty_master_fd) < 0 || unlockpt(uart->pty_master_fd) < 0) {
            perror("Failed to grant/unlock PTY master");
            close(uart->pty_master_fd);
            pthread_mutex_destroy(&uart->tx_mutex);
            pthread_mutex_destroy(&uart->rx_mutex);
            free(uart->tx_buffer);
            free(uart->rx_buffer);
            return NULL;
        }
    }

    int flags = fcntl(uart->pty_master_fd, F_GETFL, 0);
    fcntl(uart->pty_master_fd, F_SETFL, flags | O_NONBLOCK);

    // Retrieve and log the PTY slave name.
    char *pty_slave = ptsname(uart->pty_master_fd);
    if (pty_slave) {
        fprintf(stderr, "UART PTY master fd %d connected to PTY slave: %s\n", uart->pty_master_fd, pty_slave);
    } else {
        fprintf(stderr, "UART: Could not retrieve PTY slave name\n");
    }

    // Ensure a valid baud rate; default to 9600 baud if not specified.
    if (uart->config.baud_rate == 0) {
        uart->config.baud_rate = 9600;
    }
    unsigned int byte_delay_us = compute_byte_delay(uart->config.baud_rate);
    fprintf(stderr, "UART configured with baud rate %u, byte delay: %u microseconds\n", uart->config.baud_rate, byte_delay_us);

    // Register the cleanup handler.
    pthread_cleanup_push(uart_cleanup, uart);

    // Main loop: process PTY I/O.
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
            enqueue_interrupt(((AppState*)arg)->state->i_queue, UART_IRQ_RX);

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
                enqueue_interrupt(((AppState*)arg)->state->i_queue, UART_IRQ_TX);
            }

            // Simulate the delay for transmitting one byte.
            usleep(byte_delay_us);
        } else {
            pthread_mutex_unlock(&uart->tx_mutex);
        }
        pthread_testcancel();
    }

    // Execute the cleanup handler before returning.
    pthread_cleanup_pop(1);
    return NULL;
}

//-------------------------------
// Write a byte to the UART transmit buffer
//-------------------------------
void uart_write(UART *uart, uint8_t data) {
    pthread_mutex_lock(&uart->tx_mutex);

    // Calculate next head index in the circular buffer.
    size_t next_head = (uart->tx_head + 1) % uart->tx_buffer_size;
    // Check if buffer is full.
    if (next_head == uart->tx_tail) {
        fprintf(stderr, "TX buffer full, dropping data\n");
        pthread_mutex_unlock(&uart->tx_mutex);
        return;
    }
    uart->tx_buffer[uart->tx_head] = data;
    uart->tx_head = next_head;
    pthread_mutex_unlock(&uart->tx_mutex);
}

//-------------------------------
// Read a byte from the UART receive buffer
//-------------------------------
bool uart_read(UART *uart, uint8_t *data) {
    bool ret_val = false;
    pthread_mutex_lock(&uart->rx_mutex);
    // Check if RX buffer is empty.
    if (uart->rx_head == uart->rx_tail) {
        ret_val = false;
    } else {
        *data = uart->rx_buffer[uart->rx_tail];
        uart->rx_tail = (uart->rx_tail + 1) % uart->rx_buffer_size;
        ret_val = true;
    }
    pthread_mutex_unlock(&uart->rx_mutex);
    return ret_val;
}
