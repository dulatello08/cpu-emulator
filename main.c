#include "main.h"

#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>
#include <pthread.h>

#include "uart/uart.h"

#define SOCKET_PATH "/tmp/emulator.sock"

void sigintHandler(__attribute__((unused)) int signal) {
    fflush(stdout);
}

typedef void (*CommandFunc)(AppState *appState, const char *args);

typedef struct {
    const char *command;
    CommandFunc func;
} Command;

void command_start(AppState *appState, __attribute__((unused)) const char *args);
void command_stop(AppState *appState, __attribute__((unused)) const char *args);
void command_program(AppState *appState, const char *args);
void command_flash(AppState *appState, const char *args);
void command_help(__attribute__((unused)) AppState *appState, __attribute__((unused)) const char *args);
void command_exit(__attribute__((unused)) AppState *appState, __attribute__((unused)) const char *args);
void command_interrupt(AppState *appState, const char *args);
void load_config(AppState *appState, const char *filename);
void display_config(const MemoryConfig *config);

// Command to preview current memory configuration
void command_view_config(AppState *appState, __attribute__((unused)) const char *args);

// Command to reload configuration at runtime
void command_reload_config(AppState *appState, const char *args);

const Command COMMANDS[] = {
        {"start", command_start},
        {"stop", command_stop},
        {"program", command_program},
        {"flash", command_flash},
        {"help", command_help},
        {"h", command_help},
        {"exit", command_exit},
        {"interrupt", command_interrupt},
        {"config_show", command_view_config},
        {"config", command_reload_config},
        {NULL, NULL}
};

AppState *new_app_state(void) {
    AppState *appState = malloc(sizeof(AppState));

    appState->state = mmap(NULL, sizeof(CPUState), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    appState->state->reg = mmap(NULL, 16 * sizeof(uint16_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    memset(appState->state->reg, 0, 16 * sizeof(uint16_t));
    appState->emulator_running = mmap(NULL, 1, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *appState->emulator_running = 0;
    appState->emulator_thread = 0;
    appState->state->page_table = create_page_table();
    appState->state->i_vector_table = init_interrupt_vector_table();
    appState->state->i_queue = init_interrupt_queue();
    appState->state->uart = calloc(1, sizeof(UART));
    if (appState->state->uart) {
        // Set buffer sizes and initial values.
        appState->state->uart->tx_buffer_size = 64;
        appState->state->uart->rx_buffer_size = 64;
        appState->state->uart->pty_master_fd = -1;  // Not opened yet.
        appState->state->uart->running = false;
    }

    return appState;
}

void free_app_state(AppState *appState) {
    if (*(appState->emulator_running) != 0) {
        pthread_cancel(appState->emulator_thread);
        pthread_join(appState->emulator_thread, NULL);
    }
    munmap(appState->emulator_running, 1);
    munmap(appState->state->reg, 16 * sizeof(uint16_t));
    free_all_pages(appState->state->page_table);
    free(appState->state->i_vector_table);
    free(appState->state->i_queue);
    free(appState->state->uart);
    munmap(appState->state, sizeof(CPUState));
    // if (appState->gui_pid) {
    //     munmap(appState->gui_shm, sizeof(gui_process_shm_t));
    //     close(appState->gui_shm_fd);
    //     shm_unlink("emulator_gui_shm");
    // }
    free(appState);
}

void* emulator_thread_func(void* arg) {
    AppState *appState = (AppState*) arg;

    // Set cancellation type as needed.
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    // Start the UART thread if a UART instance is present.
    if (appState->state->uart) {
        // Make sure the UART running flag is set.
        appState->state->uart->running = true;
        if (pthread_create(&appState->state->uart_thread, NULL, uart_start, appState) != 0) {
            perror("Failed to create UART thread");
        }
    }

    // Start the emulator main loop.
    start(appState);

    // When the emulator stops, signal the UART thread to stop.
    if (appState->state->uart) {
        appState->state->uart->running = false;
        pthread_cancel(appState->state->uart_thread);
        pthread_join(appState->state->uart_thread, NULL);
    }

    *(appState->emulator_running) = 0;
    pthread_exit(NULL);
}

void command_start(AppState *appState, __attribute__((unused)) const char *args){
    if (*(appState->emulator_running) == 0) {
        pthread_t emulator_thread;
        *(appState->emulator_running) = 1;
        if(pthread_create(&emulator_thread, NULL, emulator_thread_func, appState) != 0) {
            perror("Failed to create emulator thread");
            *(appState->emulator_running) = 0;
            return;
        }
        appState->emulator_thread = emulator_thread;
    } else {
        printf("Emulator already running.\n");
    }
}

void command_stop(AppState *appState, __attribute__((unused)) const char *args) {
    if (*(appState->emulator_running) == 0) {
        printf("Emulator is not running.\n");
        return;
    }

    if (pthread_cancel(appState->emulator_thread) == 0) {
        // The cancel operation was successful
        printf("Emulator successfully stopped.\n");
        *(appState->emulator_running) = 0;
    } else {
        // An error occurred during the cancel operation
        perror("Error stopping emulator");
    }
}

void execute_command(AppState *appState, const char *command, const char *args) {
    if (!appState || !command) {
        fflush(stdout);
        return;
    }

    for (const Command *cmd = COMMANDS; cmd->command != NULL; cmd++) {
        if (cmd->command && strcmp(command, cmd->command) == 0 && cmd->func) {
            cmd->func(appState, args);
            return;
        }
    }

    printf("Unknown command. Type help or h for help.\n");
}

int main(int argc, char *argv[]) {
    // Set the SIGINT (Ctrl+C) signal handler to sigintHandler
    signal(SIGINT, sigintHandler);
    AppState *appState = new_app_state();
    char *config_file = "config.ini";
    // Parse arguments
    int opt;
    while ((opt = getopt(argc, argv, "p:m:c:")) != -1) {
        switch (opt) {
            case 'p':
                appState->program_file = optarg;
                break;
            case 'm':
                appState->flash_file = optarg;
                break;
            case 'c':
                config_file = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s [-p program_file] [-m flash_file] [-c config_file]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    load_config(appState, config_file);

    uint8_t *program_memory;
    appState->program_size = load_program(appState->program_file, &program_memory);
    initialize_page_table(appState->state, program_memory, appState->program_size);
    free(program_memory);
    printf("Loaded program %lu bytes\n", appState->program_size);

    char input[MAX_INPUT_LENGTH];
    while(1) {
        printf(">> ");
        if (fgets(input, MAX_INPUT_LENGTH, stdin) == NULL) {
            printf("\n");
            break;
        }
        char *command = strtok(input, " \n");
        char *args = strtok(NULL, "\n");
        execute_command(appState, command, args);
        fflush(stdout);
        usleep(500000);
    }

    free_app_state(appState);
    return 0;
}

void command_exit(__attribute__((unused)) AppState *appState, __attribute__((unused)) const char *args){
    printf("Exiting emulator...\n");

    free_app_state(appState);
    exit(0);
}


void command_interrupt(AppState *appState, const char *args) {
    if (args == NULL || *args == '\0') {
        printf("Usage: interrupt <source>\n");
        return;
    }

    char *endptr;
    unsigned long source = strtoul(args, &endptr, 0);
    if (endptr == args || *endptr != '\0') {
        printf("Invalid interrupt source: %s\n", args);
        return;
    }

    if (enqueue_interrupt(appState->state->i_queue, (uint8_t)source)) {
        printf("Interrupt %lu enqueued.\n", source);
    } else {
        printf("Interrupt queue full; cannot enqueue interrupt %lu.\n", source);
    }
}

void command_help(__attribute__((unused)) AppState *appState, __attribute__((unused)) const char *args) {
    printf("Commands:\n");
    printf("start - start emulator\n");
    printf("stop - stop emulator \n");
    printf("program <filename> - load program\n");
    printf("flash <filename> - load flash\n");
    printf("ctl_l or ctl_listen- start listening for connections on Unix socket\n");
    printf("help or h - display this help message\n");
    // printf("exit - exit the program\n");
    // clear_display(appState->gui_shm->display);
    // write_to_display(appState->gui_shm->display, 0x41);
    // kill(appState->gui_pid, SIGUSR1);
}

void command_program(AppState *appState, __attribute__((unused)) const char *args){
    appState->program_file = (char*) args;
    uint8_t *program_memory;
    appState->program_size = load_program(appState->program_file, &program_memory);
    initialize_page_table(appState->state, program_memory, appState->program_size);
    free(program_memory);
    printf("Loaded program %lu bytes\n", appState->program_size);
}

void command_flash(__attribute__((unused)) AppState *appState, __attribute__((unused)) const char *args){
//    const char* filename = args;
}

// Function to load the configuration file into appState
void load_config(AppState *appState, const char *filename) {
    if (parse_ini_file(filename, &appState->state->memory_config) == 0) {
        printf("Configuration loaded from %s\n", filename);
    } else {
        fprintf(stderr, "Error: Could not load configuration from %s\n", filename);
    }
}

// Function to display the current configuration
void display_config(const MemoryConfig *config) {
    printf("Current Memory Configuration:\n");
    for (size_t i = 0; i < config->section_count; i++) {
        printf("Section: %s\n", config->sections[i].section_name);
        printf("  Type: %d\n", config->sections[i].type);
        printf("  Start Address: 0x%X\n", config->sections[i].start_address);
        printf("  Page Count: %u\n", config->sections[i].page_count);
        if (config->sections[i].type == MMIO_PAGE) {
            printf("  Device: %s\n", config->sections[i].device);
        }
    }
}

void command_view_config(AppState *appState, __attribute__((unused)) const char *args) {
    display_config(&appState->state->memory_config);
}

void command_reload_config(AppState *appState, const char *args) {
    if (parse_ini_file(args, &appState->state->memory_config) == 0) {
        printf("Configuration reloaded from %s\n", args);
    } else {
        printf("Error: Could not reload configuration from %s\n", args);
    }
}
