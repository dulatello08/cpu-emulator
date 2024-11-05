#include "main.h"

#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>
#include <pthread.h>

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
void command_input(AppState *appState, const char *args);
void command_print(AppState *appState, __attribute__((unused)) const char *args);
void command_exit(__attribute__((unused)) AppState *appState, __attribute__((unused)) const char *args);
void command_ctl_listen(__attribute__((unused)) AppState *appState, __attribute__((unused)) __attribute__((unused)) const char *args);
void command_interrupt(AppState *appState, const char *args);
void command_gui(AppState *appState, __attribute__((unused)) const char *args);
void command_gui_and_start(AppState *appState, __attribute__((unused)) const char *args);
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
        {"input", command_input},
        {"print", command_print},
        {"exit", command_exit},
        {"ctl_listen", command_ctl_listen},
        {"ctl_l", command_ctl_listen},
        {"interrupt", command_interrupt},
        {"gui", command_gui},
        {"g", command_gui},
        {"gs", command_gui_and_start},
        {"config_show", command_view_config},
        {"config", command_reload_config},
        {NULL, NULL}
};

AppState *new_app_state(void) {
    AppState *appState = malloc(sizeof(AppState));

    appState->state = mmap(NULL, sizeof(CPUState), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    appState->state->reg = mmap(NULL, 16 * sizeof(uint16_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    appState->emulator_running = mmap(NULL, 1, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *appState->emulator_running = 0;
    appState->emulator_thread = 0;
    appState->gui_shm = NULL;
    appState->state->page_table = create_page_table();

    return appState;
}

void free_app_state(AppState *appState) {
    pthread_cancel(appState->emulator_thread);

    munmap(appState->emulator_running, 1);
    munmap(appState->state->reg, 16 * sizeof(uint16_t));
    munmap(appState->state->i_queue->sources, *(appState->state->i_queue->size) * sizeof(uint8_t));
    munmap(appState->state->i_queue->size, sizeof(uint8_t));
    munmap(appState->state->i_queue, sizeof(InterruptQueue));
    munmap(appState->state, sizeof(CPUState));
    if (appState->gui_pid) {
        munmap(appState->gui_shm, sizeof(gui_process_shm_t));
        close(appState->gui_shm_fd);
        shm_unlink("emulator_gui_shm");
    }
    free(appState);
}

void* emulator_thread_func(void* arg) {
    AppState *appState = (AppState*) arg;
    start(appState);
    printf(">> ");
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

    char input[MAX_INPUT_LENGTH];
    load_config(appState, config_file);
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

void command_ctl_listen(__attribute__((unused)) AppState *appState, __attribute__((unused)) const char *args) {
#ifdef EMULATOR_SOCKET
    // Remove existing socket file
    unlink(SOCKET_PATH);
    int server_fd;
    struct sockaddr_un server_addr;

    // Create socket
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation");
        return;
    }

    // Set up server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Socket bind");
        close(server_fd);
        return;
    }

    // Listen for connections
    if (listen(server_fd, 5) == -1) {
        perror("Socket listen");
        close(server_fd);
        return;
    }

    // Fork a new process to handle the socket connections
    pid_t child_pid = fork();
    if (child_pid == -1) {
        perror("Fork failed");
        close(server_fd);
        return;
    } else if (child_pid == 0) {  // Child process
        // Close server_fd in the child process
        // Accept and handle connections
        printf("Accepting connections on %s\n", SOCKET_PATH);
        while (1) {
            int client_fd;
            struct sockaddr_un client_addr;
            socklen_t client_addr_len = sizeof(client_addr);

            client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
            if (client_fd == -1) {
                perror("Socket accept");
                continue;  // Continue accepting other connections
            }

            // Handle the connection
            handle_connection(client_fd, (CPUState *) appState->state, appState->shared_data_memory);

            close(client_fd);
        }

        // Child process should not reach here
        exit(0);
    } else {  // Parent process
        // Close server_fd in the parent process
        close(server_fd);
    }
#else
    printf("Feature not enabled\n");
#endif
}

void command_exit(__attribute__((unused)) AppState *appState, __attribute__((unused)) const char *args){
    printf("Exiting emulator...\n");
    free_app_state(appState);
    exit(0);
}


void command_interrupt(AppState *appState, const char *args) {
    uint8_t source = strtoul(args, NULL, 0);
    //kill(appState->emulator_pid, SIGSTOP);
    printf("main pointer: %p\n", (void *) appState->state->i_queue);
    push_interrupt(appState->state->i_queue, source);
    //printf("result: %02x\n", pop_interrupt(appState->state->i_queue));
    //kill(appState->emulator_pid, SIGCONT);
}

void command_gui(AppState *appState,  __attribute__((unused)) const char *args) {
    open_gui(appState);
}

void command_gui_and_start(AppState *appState, __attribute__((unused)) const char * args) {
    open_gui(appState);
    usleep(1000);
    command_start(appState, args);
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

void command_input(__attribute__((unused)) AppState *appState, const char *args) {
    if (args == NULL || *args == '\0') {
        printf("Error: Empty input\n");
        return;
    }

    const char *arg = args;

    // Check if input starts with "0x" or "0X", and skip the prefix if present
    if (strncmp(arg, "0x", 2) == 0 || strncmp(arg, "0X", 2) == 0) {
        arg += 2;
    }

    char *endptr;
    unsigned long value = strtoul(arg, &endptr, 16);

    if (endptr == arg || *endptr != '\0' || value > 0xFF) {
        printf("Error: Invalid hexadecimal byte. Provided: %s\n", args);
    } else {
//        appState->shared_data_memory[254] = (uint8_t)value;
    }
}

void command_print(AppState *appState, __attribute__((unused)) const char *args){
    print_display(appState->state->display);
}

void command_program(AppState *appState, __attribute__((unused)) const char *args){
//    const char* filename = args;

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
