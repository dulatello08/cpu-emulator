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
#define MAX_QUEUE_SIZE 10

#ifdef __cplusplus
extern "C" {
#endif

    uint8_t sdlToCpuCode(int scanCode);

    typedef struct {
        char display[LCD_WIDTH][LCD_HEIGHT];
        uint8_t keyboard_o[3];
    } gui_process_shm_t;

    typedef struct {
        char display[LCD_WIDTH][LCD_HEIGHT];
        SDL_Renderer *renderer;
        TTF_Font *font;
        int updateQueue[MAX_QUEUE_SIZE]; // Custom queue for update requests
        int queueHead; // Index of the first element
        int queueTail; // Index of the last element
        int queueCount; // Number of elements in queue
    } signal_handler_data_t;
#ifdef __cplusplus
}
#endif

#endif //GUI_MAIN_H
