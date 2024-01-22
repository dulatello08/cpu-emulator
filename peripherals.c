//
// Created by dulat on 2/19/23.
//

#include "main.h"

void clear_display(char display[LCD_WIDTH][LCD_HEIGHT]) {
    // Clear the display by setting all characters to \0
    for (int i = 0; i < LCD_WIDTH; i++) {
        for (int j = 0; j < LCD_HEIGHT; j++) {
            display[i][j] = '\0';
        }
    }
}

void print_display(char display[LCD_WIDTH][LCD_HEIGHT]) {
    // Print the display to the console
    for (int j = 0; j < LCD_HEIGHT; j++) {
        for (int i = 0; i < LCD_WIDTH; i++) {
            char character = display[i][j];
            // Replace null character with '*'
            if (character == '\0') {
                character = '*';
            }
            printf("%c", character);
        }
        printf("\n");
    }
}

void write_to_display(char display[LCD_WIDTH][LCD_HEIGHT], uint8_t data) {
    static int x = 0;
    static int y = 0;

    if (data >= 0x20 && data <= 0x7e) {
        // Printable ASCII character
        display[x][y] = (char) data;
        x++;
        if (x == LCD_WIDTH) {
            // Move to next line if we reach the end of the row
            x = 0;
            y++;
            if (y == LCD_HEIGHT) {
                // If we reach the end of the display, wrap around to the top
                y = 0;
            }
        }
    } else if (data == 0x0A) {
        // Line Feed character
        x = 0;
        y++;
        if (y == LCD_HEIGHT) {
            // If we reach the end of the display, wrap around to the top
            y = 0;
        }
    } else if (data == 0x01) {
        // Clear display
        clear_display(display);
        x = 0;
        y = 0;
    } else if (data == 0x02) {
        // Return home
        x = 0;
        y = 0;
    }
}
