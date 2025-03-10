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
    uint8_t sources[10];
    uint8_t size; // Index of the top element
} GuiInterruptQueue;

// GUI shared memory type
typedef struct {
    char display[LCD_WIDTH][LCD_HEIGHT];
    uint8_t keyboard_o[2];
    GuiInterruptQueue i_queue;
} gui_process_shm_t;

inline void push_interrupt(GuiInterruptQueue* queue, uint8_t source) {
    if(queue->size == MAX_QUEUE_SIZE) {
        return;
    } //todo fix macros
    if(queue->size > 0) {
        for (uint8_t i = queue->size; i > 0; i--) {
            queue->sources[i] = queue->sources[i - 1];
        }
    }
    queue->sources[0] = source;
    queue->size++;

    printf("GUI int queue. Size: %d Queue: ", queue->size);

    for (int i = 0; i < queue->size; i++) {
        printf("%02x ", queue->sources[i]);
    }
    printf("\n");
}
#ifdef __cplusplus
}
#endif

#endif //GUI_MAIN_H
