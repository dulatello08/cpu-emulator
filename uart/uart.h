//
// uart.h
// Created by Dulat S on 3/7/25.
//

#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <pthread.h>
#include "common.h"  // Get shared definitions (including AppState, CPUState, etc.)

// ----------------------------
// UART Definitions
// ----------------------------

// UART configuration structure.
typedef struct {
    uint32_t baud_rate;
} UARTConfig;

// UART structure definition.
typedef struct UART {
    // Configuration and state.
    UARTConfig config;

    // Simulated registers for status, transmit, and receive.
    uint32_t status_reg;
    uint8_t tx_reg;
    uint8_t rx_reg;

    // Mutexes for protecting TX and RX buffers and registers.
    pthread_mutex_t tx_mutex;
    pthread_mutex_t rx_mutex;

    // TX and RX buffers (using fixed-size circular buffers).
    uint8_t *tx_buffer;
    size_t tx_buffer_size;
    size_t tx_head;
    size_t tx_tail;

    uint8_t *rx_buffer;
    size_t rx_buffer_size;
    size_t rx_head;
    size_t rx_tail;

    // File descriptor for the PTY master interface.
    int pty_master_fd;

    // Running flag to control the UART thread's lifecycle.
    bool running;
} UART;

/**
 * @brief Start the UART processing thread.
 *
 * @param arg Pointer to app state instance.
 */
void *uart_start(void *arg);

/**
 * @brief Write a byte to the UART transmit buffer.
 *
 * @param uart Pointer to a UART instance.
 * @param data The byte to be transmitted.
 */
void uart_write(UART *uart, uint8_t data);

/**
 * @brief Read a byte from the UART receive buffer.
 *
 * @param uart Pointer to a UART instance.
 * @param data Pointer where the received byte will be stored.
 * @return true if a byte was read successfully, false if the buffer is empty.
 */
bool uart_read(UART *uart, uint8_t *data);

#endif // UART_H
