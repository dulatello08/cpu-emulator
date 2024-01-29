#include "main.h"
#include <fcntl.h>
#include <sys/stat.h>

void open_gui(AppState *appState) {
    if (*(appState->emulator_running)) {
        printf("Please restart the emulator to use the GUI subsystem.\n");
        return; // Exit the function to prevent further execution
    }

    const char *memname = "emulator_gui_shm";
    const size_t size = sizeof(gui_process_shm_t);

    // Unlink first to ensure the shared memory segment can be resized
    shm_unlink(memname);

    // Create and open a shared memory object in the parent process
    int memFd = shm_open(memname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (memFd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Check the current size of the shared memory object
    struct stat mapstat;
    if (-1 != fstat(memFd, &mapstat)) {
        // Resize only if the current size is zero
        if (mapstat.st_size == 0) {
            if (ftruncate(memFd, size) == -1) {
                perror("ftruncate");
                exit(EXIT_FAILURE);
            }
        }
    } else {
        perror("fstat");
        exit(EXIT_FAILURE);
    }

    // Map the shared memory object
    void *shared_memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, memFd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Store the shared memory pointer in AppState
    appState->gui_shm = (gui_process_shm_t *) shared_memory;

    appState->gui_pid = fork();

    if (appState->gui_pid == -1) {
        // Handle error
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (appState->gui_pid == 0) {
        // Child process

        // Pass the shared memory name to the child process
        execlp("build/gui_subsystem", "gui_subsystem", (char *) NULL);
        // Exec only returns on error
        perror("exec");
        exit(EXIT_FAILURE);
    } else {
        memset(appState->gui_shm->keyboard_o, 0, sizeof(appState->gui_shm->keyboard_o));
        clear_display(appState->gui_shm->display);
        appState->gui_shm_fd = memFd;
        // Parent process continues...
    }
}
