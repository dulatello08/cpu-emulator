//
// uart.c
// Created by Dulat S on 3/7/25.
//

#include "uart.h"
#include <pthread.h>
#include <unistd.h>

#ifdef __APPLE__
    #include <util.h>           // posix_openpt, grantpt, unlockpt on macOS
#else
    #ifdef __linux__
        /* glibc sometimes omits prototypes unless certain feature macros
           are defined; fall back to externs so the code still builds. */
        extern int posix_openpt(int flags);
        extern int grantpt(int fd);
        extern int unlockpt(int fd);
    #endif
#endif

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>               // nanosleep
#include "main.h"

/* uart.c must come before any system header on some platforms */
#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__)
    #define _XOPEN_SOURCE 600
    #include <pty.h>            // posix_openpt, grantpt, unlockpt, ptsname
#endif

/* -------------------------------------------------------------------------- */
/* Configuration                                                               */
/* -------------------------------------------------------------------------- */

#define UART_IRQ_RX            0
#define UART_IRQ_TX            1

#define IDLE_SLEEP_US          1000      /* 1 ms when line is idle           */
#define EIO_SLEEP_MS           250       /* 250 ms when slave not yet open   */

/* -------------------------------------------------------------------------- */
/* Helpers                                                                     */
/* -------------------------------------------------------------------------- */

static inline unsigned int compute_byte_delay(uint32_t baud_rate)
/* μs to transmit one byte: 1 start + 8 data + 1 stop */
{
    if (baud_rate == 0) baud_rate = 115200;
    return (unsigned int)(10.0 / baud_rate * 1e6);
}

static inline void sleep_ms(unsigned int ms)
/* cross-platform tiny wrapper around nanosleep */
{
    struct timespec ts = {
        .tv_sec  = ms / 1000,
        .tv_nsec = (ms % 1000) * 1000000UL
    };
    nanosleep(&ts, NULL);
}

static void uart_cleanup(void *arg)
/* called automatically on pthread_cancel or normal return */
{
    UART *uart = (UART *)arg;

    if (uart->pty_master_fd >= 0) {
        close(uart->pty_master_fd);
        uart->pty_master_fd = -1;
    }
    free(uart->tx_buffer); uart->tx_buffer = NULL;
    free(uart->rx_buffer); uart->rx_buffer = NULL;

    pthread_mutex_destroy(&uart->tx_mutex);
    pthread_mutex_destroy(&uart->rx_mutex);

    fprintf(stderr, "UART cleanup completed.\n");
}

/* -------------------------------------------------------------------------- */
/* UART thread                                                                 */
/* -------------------------------------------------------------------------- */

void *uart_start(void *arg)
{
    UART *uart = ((AppState *)arg)->state->uart;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    fprintf(stderr, "UART thread started.\n");

    uart->status_reg   = 0;
    uart->tx_head = uart->tx_tail = 0;
    uart->rx_head = uart->rx_tail = 0;

    /* allocate circular buffers if caller didn’t */
    if (!uart->tx_buffer &&
        !(uart->tx_buffer = malloc(uart->tx_buffer_size))) {
        perror("UART: alloc TX buffer");
        return NULL;
    }
    if (!uart->rx_buffer &&
        !(uart->rx_buffer = malloc(uart->rx_buffer_size))) {
        perror("UART: alloc RX buffer");
        free(uart->tx_buffer);
        return NULL;
    }

    /* init mutexes */
    if (pthread_mutex_init(&uart->tx_mutex, NULL) ||
        pthread_mutex_init(&uart->rx_mutex, NULL)) {
        perror("UART: init mutexes");
        uart_cleanup(uart);
        return NULL;
    }

    /* open PTY master if caller didn’t */
    if (uart->pty_master_fd < 0) {
        uart->pty_master_fd = posix_openpt(O_RDWR | O_NOCTTY);
        if (uart->pty_master_fd < 0 ||
            grantpt(uart->pty_master_fd)  < 0 ||
            unlockpt(uart->pty_master_fd) < 0) {
            perror("UART: open/grant/unlock PTY master");
            uart_cleanup(uart);
            return NULL;
        }
    }

    int flags = fcntl(uart->pty_master_fd, F_GETFL, 0);
    fcntl(uart->pty_master_fd, F_SETFL, flags | O_NONBLOCK);

    char *pty_slave = ptsname(uart->pty_master_fd);
    fprintf(stderr, "UART PTY master fd %d -> %s\n",
            uart->pty_master_fd, pty_slave ? pty_slave : "(unknown)");

    if (uart->config.baud_rate == 0) uart->config.baud_rate = 9600;
    unsigned int byte_delay_us = compute_byte_delay(uart->config.baud_rate);
    fprintf(stderr, "UART baud %u, byte delay %u µs\n",
            uart->config.baud_rate, byte_delay_us);

    pthread_cleanup_push(uart_cleanup, uart);   /* auto-cleanup on exit */

    /* ------------------------------------------------------------------ */
    /* main loop                                                          */
    /* ------------------------------------------------------------------ */
    while (uart->running) {
        bool did_io      = false;
        bool got_eio     = false;      /* true if master is open but slave isn’t */

        /* ----------------- RX ----------------- */
        uint8_t in_byte;
        ssize_t n = read(uart->pty_master_fd, &in_byte, 1);

        if (n > 0) {
            did_io = true;
            pthread_mutex_lock(&uart->rx_mutex);
            size_t next_head = (uart->rx_head + 1) % uart->rx_buffer_size;
            if (next_head != uart->rx_tail) {
                uart->rx_buffer[uart->rx_head] = in_byte;
                uart->rx_head = next_head;
                uart->status_reg |= 0x01;                /* RX ready */
            } else {
                fprintf(stderr, "UART RX buffer overflow\n");
            }
            pthread_mutex_unlock(&uart->rx_mutex);

            enqueue_interrupt(((AppState *)arg)->state->i_queue,
                              UART_IRQ_RX);
            usleep(byte_delay_us);
        }
        else if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* nothing to read right now — handled below */
            } else if (errno == EIO) {
                /* slave side not yet open */
                got_eio = true;
            } else {
                perror("UART read failed");
            }
        }

        /* ----------------- TX ----------------- */
        pthread_mutex_lock(&uart->tx_mutex);
        if (uart->tx_tail != uart->tx_head) {
            did_io = true;

            uint8_t out_byte = uart->tx_buffer[uart->tx_tail];
            uart->tx_tail = (uart->tx_tail + 1) % uart->tx_buffer_size;
            pthread_mutex_unlock(&uart->tx_mutex);

            ssize_t w = write(uart->pty_master_fd, &out_byte, 1);
            if (w > 0) {
                uart->status_reg |= 0x02;                /* TX complete */
                enqueue_interrupt(((AppState *)arg)->state->i_queue,
                                  UART_IRQ_TX);
                usleep(byte_delay_us);
            } else if (w < 0 && errno == EIO) {
                /* slave still closed: put byte back and wait */
                got_eio = true;
                /* roll tail index back so data is retried later */
                pthread_mutex_lock(&uart->tx_mutex);
                uart->tx_tail =
                    (uart->tx_tail + uart->tx_buffer_size - 1) %
                    uart->tx_buffer_size;
                pthread_mutex_unlock(&uart->tx_mutex);
            } else if (w < 0) {
                perror("UART write failed");
            }
        } else {
            pthread_mutex_unlock(&uart->tx_mutex);
        }

        /* ------------- idle & slave-closed handling -------------- */
        if (!did_io) {
            if (got_eio) {
                /* slave side missing: back off much longer */
                sleep_ms(EIO_SLEEP_MS);
            } else {
                /* normal idle */
                usleep(IDLE_SLEEP_US);
            }
        }

        pthread_testcancel();          /* keep cancel responsive */
    }

    pthread_cleanup_pop(1);            /* invokes uart_cleanup */
    return NULL;
}

/* -------------------------------------------------------------------------- */
/* Public helpers                                                              */
/* -------------------------------------------------------------------------- */

void uart_write(UART *uart, uint8_t data)
{
    pthread_mutex_lock(&uart->tx_mutex);
    size_t next_head = (uart->tx_head + 1) % uart->tx_buffer_size;
    if (next_head == uart->tx_tail) {
        fprintf(stderr, "UART TX buffer full — dropping byte\n");
        pthread_mutex_unlock(&uart->tx_mutex);
        return;
    }
    uart->tx_buffer[uart->tx_head] = data;
    uart->tx_head = next_head;
    pthread_mutex_unlock(&uart->tx_mutex);
}

bool uart_read(UART *uart, uint8_t *data)
{
    bool have_byte = false;
    pthread_mutex_lock(&uart->rx_mutex);
    if (uart->rx_head != uart->rx_tail) {
        *data = uart->rx_buffer[uart->rx_tail];
        uart->rx_tail = (uart->rx_tail + 1) % uart->rx_buffer_size;
        have_byte = true;
    }
    pthread_mutex_unlock(&uart->rx_mutex);
    return have_byte;
}
