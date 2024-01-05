#include "main.h"
#include <keyboard/main.h>
#include <stdint.h>

#define PAIR_BW       1
#define BRIGHT_WHITE  15

void keyboard_mode(__attribute__((unused)) AppState *appState) {
    keyboard(&appState->state->memory[appState->state->mm.peripheralControl.startAddress + 4], &appState->state->memory[appState->state->mm.peripheralControl.startAddress + 5]);
}

void tty_mode(AppState *appState) {

    //dark magic
    (void)appState;

    initscr();          // Initialize ncurses
    cbreak();           // Line buffering disabled
    noecho();           // Don't echo user input
    nodelay(stdscr, true);
    curs_set(FALSE);    // Hide cursor
    keypad(stdscr, TRUE); // Enable special keys like arrow keys
    start_color();
    use_default_colors();
    // Create a color pair for black on white
    if (can_change_color() && COLORS >= 16)
        init_color(BRIGHT_WHITE, 1000,1000,1000);

    if (COLORS >= 16) {
        init_pair(PAIR_BW, COLOR_BLACK, BRIGHT_WHITE);
    } else {
        init_pair(PAIR_BW, COLOR_BLACK, COLOR_WHITE);
    }

    int height, width;
    getmaxyx(stdscr, height, width);

    // Calculate the height for the bottom window (10% of the total height)
    int bottomHeight = height * 0.10;

    // Create a window for the top portion (80% of the height)
    WINDOW *topWin = newwin(height - bottomHeight - 1, width, 0, 0);

    // Create a window for the status line (10% of the height)
    WINDOW *statusWin = newwin(1, width, height - bottomHeight - 1, 0);

    // Create a window for the bottom portion (10% of the height)
    WINDOW *bottomWin = newwin(bottomHeight, width, height - bottomHeight, 0);

    char commandBuffer[256];
    int commandBufferIndex = 0;

    // Status message initialization
    char statusMessage[256];
    strcpy(statusMessage, "TTY State");
    wclear(statusWin);
    wprintw(statusWin, "Status: %s", statusMessage);
    wbkgd(statusWin, COLOR_PAIR(PAIR_BW));
    wattron(statusWin, COLOR_PAIR(PAIR_BW));
    wrefresh(statusWin);

    // Command mode flag
    int commandMode = 0;

    // Wait for user input and exit on 'q' keypress
    int ch;
    while ((ch = getch()) != 4) {
        if (commandMode) {
            // Handle command mode
            if (ch == 27) {  // Esc key
                // Clear the command buffer
                memset(commandBuffer, 0, sizeof(commandBuffer));
                commandBufferIndex = 0;
                wclear(bottomWin);
                wrefresh(bottomWin);

                commandMode = 0;
            } else if (ch == '\n') {
                // Execute command and provide feedback (you can implement this part)
                // For now, we'll just print the entered command.
                wprintw(topWin, "Command Entered: %s\n", commandBuffer);
                wrefresh(topWin);

                // Check if the entered command is "q" or "quit"
                if (strcmp(commandBuffer, "q") == 0 || strcmp(commandBuffer, "quit") == 0) {
                    break; // Exit the loop
                }

                // Clear the command buffer
                memset(commandBuffer, 0, sizeof(commandBuffer));
                commandBufferIndex = 0;
                wclear(bottomWin);
                wrefresh(bottomWin);

                commandMode = 0; // Exit command mode after executing
            } else if (ch == 8 || ch == 127) { // Handle backspace (8 is ASCII code for backspace, 127 is DEL)
                // Check if there are characters to delete in the command buffer
                if (commandBufferIndex > 0) {
                    // Move the cursor back in the bottom window
                    wmove(bottomWin, getcury(bottomWin), getcurx(bottomWin) - 1);
                    // Erase the character on the screen
                    wdelch(bottomWin);
                    // Erase the character in the command buffer
                    commandBuffer[--commandBufferIndex] = '\0';
                    // Refresh the window
                    wrefresh(bottomWin);
                }
            } else {
                // Append user input to the command buffer
                if (commandBufferIndex < (int) sizeof(commandBuffer) - 1) {
                    commandBuffer[commandBufferIndex++] = ch;
                }
                // Append user input to the bottom window
                waddch(bottomWin, ch);
                wrefresh(bottomWin);
            }
            // Update the status message dynamically
            // wclear(statusWin);
            // wprintw(statusWin, "Status: %s", statusMessage);
            // wrefresh(statusWin);
            // Set the background color of the window
            //wbkgd(statusWin, COLOR_PAIR(PAIR_BW));
        } else {
            // Handle normal mode
            if (ch == ':') {
                // Enter command mode
                wclear(bottomWin);
                wrefresh(bottomWin);
                // Move the cursor to the start of the command line
                wmove(bottomWin, 0, 0);
                wprintw(bottomWin, ":");
                wrefresh(bottomWin);

                commandMode = 1; // Enter command mode
            } else {
                // Handle user input or additional content updates here in normal mode
                // For example, you can implement navigation, editing, etc.
            }
        }

       
        //usleep(500000);
    }

    // Clean up and exit
    delwin(topWin);
    delwin(statusWin);
    delwin(bottomWin);
    endwin();
}
