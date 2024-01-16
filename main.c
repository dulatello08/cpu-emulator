#include "main.h"

#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>

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
void command_free(AppState *appState, __attribute__((unused)) const char *args);
void command_exit(__attribute__((unused)) AppState *appState, __attribute__((unused)) const char *args);
void command_ctl_listen(__attribute__((unused)) AppState *appState, __attribute__((unused)) __attribute__((unused)) const char *args);
void command_interrupt(AppState *appState, const char *args);
void command_keyboard(AppState *appState, const char *args);
void command_gui(AppState *appState, __attribute__((unused)) const char * args);

const Command COMMANDS[] = {
    {"start", command_start},
    {"stop", command_stop},
    {"program", command_program},
    {"flash", command_flash},
    {"help", command_help},
    {"h", command_help},
    {"input", command_input},
    {"print", command_print},
    {"free", command_free},
    {"exit", command_exit},
    {"ctl_listen", command_ctl_listen},
    {"ctl_l", command_ctl_listen},
    {"interrupt", command_interrupt},
    {"keyboard", command_keyboard},
    {"kb", command_keyboard},
    {"gui", command_gui},
    {NULL, NULL}
};

AppState *new_app_state(void) {
    AppState *appState = malloc(sizeof(AppState));
    appState->state = mmap(NULL, sizeof(CPUState), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    appState->state->reg = mmap(NULL, 16 * sizeof(uint8_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    appState->shared_data_memory = mmap(NULL, MEMORY, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    appState->state->i_queue = mmap(NULL, sizeof(InterruptQueue), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    appState->state->i_queue->size = mmap(NULL, sizeof(uint8_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    appState->state->i_queue->sources = mmap(NULL, 10, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    appState->emulator_running = mmap(NULL, 1, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *appState->emulator_running = 0;
    appState->emulator_pid = 0;

    return appState;
}

void free_app_state(AppState *appState) {
    if(appState->fpf != NULL && appState->flash_memory != NULL) {
        int num_blocks = (appState->flash_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
        for (int i = 0; i < num_blocks; i++) {
            size_t bytes_to_write = (i == num_blocks - 1) ? appState->flash_size % BLOCK_SIZE : BLOCK_SIZE;
            size_t num_written = fwrite(appState->flash_memory[i], sizeof(uint8_t), bytes_to_write, appState->fpf);
            if (num_written != bytes_to_write) {
                fprintf(stderr, "Error: Failed to write %ld bytes to output flash file for block %d.\n", bytes_to_write, i);
            }
        }
        fclose(appState->fpf);
        free(appState->flash_memory);
    }

    munmap(appState->shared_data_memory, MEMORY);
    munmap(appState->emulator_running, 1);
    munmap(appState->state->reg, 16 * sizeof(uint8_t));
    munmap(appState->state->i_queue->sources, *(appState->state->i_queue->size) * sizeof(uint8_t));
    munmap(appState->state->i_queue->size, sizeof(uint8_t));
    munmap(appState->state->i_queue, sizeof(InterruptQueue));
    destroyCPUState(appState->state);
    munmap(appState->state, sizeof(CPUState));
    free(appState->program_memory);
    free(appState);
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

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--program") == 0) {
            if (i + 1 < argc) {
                appState->program_file = argv[i + 1];
                i++;
            }
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--flash") == 0) {
            if (i + 1 < argc) {
                appState->flash_file = argv[i + 1];
                i++;
            }
        }
    }

    // Load program and flash files
    if (appState->program_file) {
        appState->program_size = load_program(appState->program_file, &appState->program_memory);
        printf("Loaded program %d bytes\n", appState->program_size);
    }
    if (appState->flash_file) {
        appState->flash_size = (int) load_flash(appState->flash_file, appState->fpf, &appState->flash_memory) + 4;
    }

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

void command_start(AppState *appState, __attribute__((unused)) const char *args){
    if (*(appState->emulator_running) == 0) {
        pid_t emulator = fork();
        appState->emulator_pid = emulator;
        *(appState->emulator_running) = 1;
        if(emulator==0) {
            if(appState->program_memory == NULL) {
                printf("Program memory not loaded\n>> ");
                *(appState->emulator_running) = 0;
                exit(1);
            }
            if(appState->flash_memory == NULL) {
                printf("Flash memory not loaded\n>> ");
                *(appState->emulator_running) = 0;
                exit(1);
            }
            start(appState);
            printf(">> ");
            *(appState->emulator_running) = 0;
            exit(0);
        }
    } else {
        printf("Emulator already running.\n");
    }
}

void command_stop(AppState *appState, __attribute__((unused)) const char *args) {
    if (appState->emulator_running == 0) {
        printf("Emulator is not running (PID is 0).\n");
        return;
    }

    if (kill(appState->emulator_pid, SIGKILL) == 0) {
        // The kill operation was successful
        printf("Emulator successfully stopped.\n");
        *(appState->emulator_running) = 0;
    } else {
        // An error occurred during the kill operation
        perror("Error stopping emulator");
    }
}

void command_program(AppState *appState, const char *args){
    const char* filename = args;
    appState->program_size = load_program(filename, &appState->program_memory);
    printf("Loaded program %d bytes\n", appState->program_size);
}

void command_flash(AppState *appState, const char *args){
    const char* filename = args;
    appState->flash_size = (int) load_flash(filename, appState->fpf, &appState->flash_memory);
}

void command_help(__attribute__((unused)) AppState *appState, __attribute__((unused)) const char *args) {
    printf("Commands:\n");
    printf("start - start emulator\n");
    printf("stop - stop emulator \n");
    printf("program <filename> - load program\n");
    printf("flash <filename> - load flash\n");
    printf("ctl_l or ctl_listen- start listening for connections on Unix socket\n");
    printf("help or h - display this help message\n");
    printf("free - free emulator memory\n");
    printf("exit - exit the program\n");
    //write(appState->gui_pipes.stdin_fd, "D(\"help\")\n", 11);
    FILE *stream = fdopen(appState->gui_pipes.stdin_fd, "w");


    // Use fprintf to write to the stream
    fprintf(stream, "C()\n");
    fflush(stream); // Ensure the data is sent immediately
    usleep(10000);
    fprintf(stream, "D(\"Hello world!********************\\n********************************\\n********************************\\n********************************\\n\")\n");
    fflush(stream); // Ensure the data is sent immediately
}

void command_input(AppState *appState, const char *args) {
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
        appState->shared_data_memory[254] = (uint8_t)value;
    }
}

void command_print(AppState *appState, __attribute__((unused)) const char *args){
    print_display(appState->state->display);
}

void command_free(AppState *appState, __attribute__((unused)) const char *args){
    printf("Freeing emulator memory...\n");
    memset(appState->shared_data_memory, 0, MEMORY);
}

void command_exit(__attribute__((unused)) AppState *appState, __attribute__((unused)) const char *args){
    printf("Exiting emulator...\n");
    exit(0);
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

void command_interrupt(AppState *appState, const char *args) {
    uint8_t source = strtoul(args, NULL, 0);
    //kill(appState->emulator_pid, SIGSTOP);
    printf("main pointer: %p\n", (void *) appState->state->i_queue);
    push_interrupt(appState->state->i_queue, source);
    //printf("result: %02x\n", pop_interrupt(appState->state->i_queue));
    //kill(appState->emulator_pid, SIGCONT);
}

void command_keyboard(AppState *appState, __attribute__((unused)) const char *args) {
    keyboard_mode(appState);
}

void command_gui(AppState *appState,  __attribute__((unused)) const char *args) {
    open_gui(appState);
}
