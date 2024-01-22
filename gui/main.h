//
// Created by Dulat S on 1/14/24.
//

#ifndef GUI_MAIN_H
#define GUI_MAIN_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <stdint.h>

#define LCD_WIDTH 32
#define LCD_HEIGHT 4

// In your_c_header.h
#ifdef __cplusplus
extern "C" {
#endif

    uint8_t sdlToCpuCode(int scanCode);

    typedef struct {
        char display[LCD_WIDTH][LCD_HEIGHT];
        uint8_t keyboard_o[2];
    } gui_process_shm_t;

#ifdef __cplusplus
}
#endif

#endif //GUI_MAIN_H
