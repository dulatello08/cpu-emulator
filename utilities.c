#include "main.h"
#include <stdint.h>


uint8_t load_program(const char *program_file, uint8_t **program_memory) {
    if (program_file == NULL) {
        fprintf(stderr, "Error: input file not specified.\n");
        return 0;
    }

    FILE *fpi = fopen(program_file, "rb");
    if (fpi == NULL) {
        fprintf(stderr, "Error: Failed to open input program file.\n");
        return 0;
    }

    fseek(fpi, 0, SEEK_END);
    long size = ftell(fpi);

    if (size != EXPECTED_PROGRAM_WORDS * sizeof(uint8_t)) {
        fprintf(stderr, "Error: Input program file does not contain %d bytes. It contains %ld bytes\n", EXPECTED_PROGRAM_WORDS, size);
        fclose(fpi);
        return 0;
    }

    *program_memory = malloc(sizeof(uint8_t));
    if (*program_memory == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for program memory.\n");
        fclose(fpi);
        return 0;
    }

    fseek(fpi, 0, SEEK_SET);
    uint8_t *temp;
    temp = calloc(EXPECTED_PROGRAM_WORDS, sizeof(uint8_t));
    size_t num_read = fread(temp, sizeof(uint8_t), EXPECTED_PROGRAM_WORDS, fpi);
    if (num_read != EXPECTED_PROGRAM_WORDS) {
        fprintf(stderr, "Error: Failed to read %d bytes from input program file.\n", EXPECTED_PROGRAM_WORDS);
        free(*program_memory);
        fclose(fpi);
        return 0;
    }
    uint8_t current_byte = 0;
    while (temp[current_byte] != OP_HLT) {
        *program_memory = realloc(*program_memory, sizeof(uint8_t) * (current_byte + 1));
        memcpy(&(*program_memory)[current_byte], &temp[current_byte], 1);
        current_byte++;
    }
    *program_memory = realloc(*program_memory, sizeof(uint8_t) * (current_byte + 1));
    memcpy(&(*program_memory)[current_byte], &temp[current_byte], 1);
    current_byte++;
    fclose(fpi);
    return current_byte;
}

int load_flash(const char *flash_file, FILE *fpf, uint8_t ***flash_memory) {
    if (flash_file == NULL) {
        fprintf(stderr, "Error: flash file not specified.\n");
        return 0;
    }

    fpf = fopen(flash_file, "r+b");
    if (fpf == NULL) {
        fprintf(stderr, "Error: Failed to open input flash file.\n");
        return 0;
    }

    fseek(fpf, 0, SEEK_END);
    long file_size = ftell(fpf);
    int num_blocks = ((int) file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    *flash_memory = calloc(num_blocks, sizeof(uint8_t *));
    if (*flash_memory == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for flash memory.\n");
        fclose(fpf);
        return 0;
    }

    for (int i = 0; i < num_blocks; i++) {
        (*flash_memory)[i] = calloc(BLOCK_SIZE, sizeof(uint8_t));
        if ((*flash_memory)[i] == NULL) {
            fprintf(stderr, "Error: Failed to allocate memory for flash block %d.\n", i);
            for (int j = 0; j < i; j++) {
                free((*flash_memory)[j]);
            }
            free(*flash_memory);
            fclose(fpf);
            return 0;
        }
    }

    fseek(fpf, 0, SEEK_SET);
    int non_zero_count = 0;
    for (int i = 0; i < num_blocks; i++) {
        size_t bytes_to_read = file_size > BLOCK_SIZE * (i + 1) ? BLOCK_SIZE : file_size % BLOCK_SIZE;
        size_t num_read = fread((*flash_memory)[i], sizeof(uint8_t), bytes_to_read, fpf);
        if (num_read != bytes_to_read) {
            fprintf(stderr, "Error: Failed to read %ld bytes from input flash file for block %d.\n", bytes_to_read, i);
            for (int j = 0; j < num_blocks; j++) {
                free((*flash_memory)[j]);
            }
            free(*flash_memory);
            fclose(fpf);
            return 0;
        }
        for (int j = 0; j < (int) bytes_to_read; j++) {
            non_zero_count += (*flash_memory)[i][j] != 0;
        }
        file_size -= (int) bytes_to_read;
    }

    fclose(fpf);
    return non_zero_count;
}

void increment_pc(CPUState *state, uint8_t opcode) {
    switch (opcode) {
        case OP_NOP:
        case OP_HLT:
        case OP_OSR:
        default:
            *(state->pc) += 1;
            break;
        case OP_CLZ:
        case OP_PSH:
        case OP_POP:
        case OP_RSM:
        case OP_RLD:
            *(state->pc) += 2;
            break;
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_STO:
        case OP_BRN:
        case OP_BRZ:
        case OP_BRO:
        case OP_JSR:
            *(state->pc) += 3;
            break;
        case OP_ADM:
        case OP_SBM:
        case OP_MLM:
        case OP_ADR:
        case OP_SBR:
        case OP_MLR:
        case OP_STM:
        case OP_LDM:
        case OP_BRR:
        case OP_BNR:
            *(state->pc) += 4;
            break;
    }
}

void handle_operation(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode, uint16_t (*operation)(uint8_t, uint16_t)) {
    uint16_t result;

    if(mode == 0) {
        result = operation(state->reg[operand_rd], operand2);
    } else if(mode == 1) {
        result = operation(state->reg[operand_rn], memory_access(state, 0, operand2, 0, 1));
    } else /*if(mode == 2)*/ {
        result = operation(state->reg[operand_rd], state->reg[operand_rn]);
    }

    state->v_flag = (result > UINT8_MAX);
    state->z_flag = (result == 0);

    if(state->v_flag) {
        if(mode == 0 || mode == 1) {
            state->reg[operand_rd] = UINT8_MAX;
        } else /*if(mode == 2)*/ {
            memory_access(state, UINT8_MAX, operand2, 1, 1);
        }
    } else {
        if(mode == 0 || mode == 1) {
            state->reg[operand_rd] = (uint8_t)result;
        } else /*if(mode == 2)*/ {
            memory_access(state, result, operand2, 1, 1);
        }
    }
}

uint16_t add_operation(uint8_t operand1, uint16_t operand2) {
    return (uint16_t)operand1 + operand2;
}

uint16_t subtract_operation(uint8_t operand1, uint16_t operand2) {
    return (uint16_t)operand1 - operand2;
}

uint16_t multiply_operation(uint8_t operand1, uint16_t operand2) {
    return (uint16_t)operand1 * operand2;
}

void add(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode) {
    handle_operation(state, operand_rd, operand_rn, operand2, mode, add_operation);
}

void subtract(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode) {
    handle_operation(state, operand_rd, operand_rn, operand2, mode, subtract_operation);
}

void multiply(CPUState *state, uint8_t operand_rd, uint8_t operand_rn, uint16_t operand2, uint8_t mode) {
    handle_operation(state, operand_rd, operand_rn, operand2, mode, multiply_operation);
}

// This function performs a memory access.
//
// Parameters:
//   state: A pointer to the CPU state.
//   reg: The register to be accessed.
//   address: The memory address to be accessed.
//   mode: The access mode.
//   srcDest: The source or destination of the access.
//
// Returns:
//   The value of the memory location at the specified address.

uint8_t memory_access(CPUState *state, uint8_t reg, uint16_t address, uint8_t mode, uint8_t srcDest) {
    switch (mode) {
        case 0:
            // Read mode
            if(!srcDest) {
                state->reg[reg] = state->memory[address];
            }
            break;
        case 1:
            // Write mode
            if(!srcDest) {
                handleWrite(state, address, state->reg[reg]);
                state->memory[address] = state->reg[reg];
            } else {
                handleWrite(state, address, reg);
                state->memory[address] = reg;
            }
            break;
        default:
            break;
    }
    return state->memory[address];
}

void pushStack(CPUState *state, uint8_t value) {
    uint8_t stackTop = state->memory[state->mm.stackMemory.startAddress];

    // Shift existing values up by one position
    for (uint8_t i = stackTop; i > 0; i--) {
        state->memory[state->mm.stackMemory.startAddress + i + 1] = state->memory[state->mm.stackMemory.startAddress + i];
    }

    // Store the new value at the top of the stack
    state->memory[state->mm.stackMemory.startAddress + 1] = value;
    state->memory[state->mm.stackMemory.startAddress]++;
}

uint8_t popStack(CPUState *state, uint8_t *out) {
    uint8_t stackTop = state->memory[state->mm.stackMemory.startAddress];
    uint8_t value = state->memory[state->mm.stackMemory.startAddress + 1];

    // Shift values down by one position
    for (uint8_t i = 1; i < stackTop; i++) {
        state->memory[state->mm.stackMemory.startAddress + i] = state->memory[state->mm.stackMemory.startAddress + i + 1];
    }

    state->memory[state->mm.stackMemory.startAddress]--;

    if (out != NULL) {
        *out = value;
    }

    return value;
}

void destroyCPUState(CPUState *state) {
    // Deallocate memory for the general-purpose registers
    if (state->reg != NULL) {
        free(state->reg);
    }

    // Deallocate memory for the program counter
    if (state->pc != NULL) {
        free(state->pc);
    }

    // Deallocate memory for the in_subroutine flag
    if (state->in_subroutine != NULL) {
        free(state->in_subroutine);
    }
}

void add_interrupt_vector(InterruptVector table[INTERRUPT_TABLE_SIZE], uint8_t index, uint8_t source, uint8_t handler) {
    if (index < INTERRUPT_TABLE_SIZE) {
        table[index].source = source;
        table[index].handler = handler;
    }
}

uint8_t get_interrupt_handler(const InterruptVector table[INTERRUPT_TABLE_SIZE], uint8_t source) {
    if (source < INTERRUPT_TABLE_SIZE) {
        return table[source].handler;
    }
    // Return an invalid value or handle the error as needed
    return 0xFF; // Change this to an appropriate error value
}
#define PAIR_BW       1
#define BRIGHT_WHITE  15

void tty_mode(AppState *appState) {

    //dark magic
    (void)appState;

    initscr();          // Initialize ncurses
    cbreak();           // Line buffering disabled
    noecho();           // Don't echo user input
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
    wprintw(statusWin, statusMessage);
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

        // Update the status message dynamically
        wclear(statusWin);
        wprintw(statusWin, statusMessage);
        wrefresh(statusWin);
        // Set the background color of the window
        wbkgd(statusWin, COLOR_PAIR(PAIR_BW));
    }

    // Clean up and exit
    delwin(topWin);
    delwin(statusWin);
    delwin(bottomWin);
    endwin();
}
