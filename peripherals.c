//
// Created by dulat on 2/19/23.
//

#include "main.h"

void clear_display(char display[LCD_WIDTH][LCD_HEIGHT]) {
    // Clear the display by setting all characters to space
    for (int i = 0; i < LCD_WIDTH; i++) {
        for (int j = 0; j < LCD_HEIGHT; j++) {
            display[i][j] = ' ';
        }
    }
}

void print_display(char display[LCD_WIDTH][LCD_HEIGHT]) {
    // Print the display to the console
    for (int j = 0; j < LCD_HEIGHT; j++) {
        for (int i = 0; i < LCD_WIDTH; i++) {
            printf("%c", display[i][j]);
        }
        printf("\n");
    }
}

void write_to_display(char display[LCD_WIDTH][LCD_HEIGHT], unsigned char data) {
    static int x = 0;
    static int y = 0;

    if (data >= 0x20 && data <= 0x7e) {
        // Printable ASCII character
        display[x][y] = data;
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