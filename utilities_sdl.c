#include "main.h"
#include <keyboard/main.h>

void keyboard_mode(AppState *appState) {
    keyboard(appState);
}

void open_gui(AppState *appState) {
    int stdin_pipe[2], stdout_pipe[2];

    // Create pipes
    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid == -1) {
        // Handle error
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process

        // Redirect stdin (close the writing end, duplicate the reading end)
        close(stdin_pipe[1]);
        dup2(stdin_pipe[0], STDIN_FILENO);
        close(stdin_pipe[0]);

        // Redirect stdout (close the reading end, duplicate the writing end)
        close(stdout_pipe[0]);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        close(stdout_pipe[1]);

        // Execute GUI subsystem
        execlp("build/gui_subsystem", "gui_subsystem", (char *)NULL);
        // Exec only returns on error
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        // Parent process

        // Close unused ends of the pipes
        close(stdin_pipe[0]); // Close reading end of stdin pipe
        close(stdout_pipe[1]); // Close writing end of stdout pipe

        // Store file descriptors in AppState
        appState->gui_pipes.stdin_fd = stdin_pipe[1]; // Writing end
        appState->gui_pipes.stdout_fd = stdout_pipe[0]; // Reading end
    }
}
