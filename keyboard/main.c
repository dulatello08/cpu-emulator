//
// Created by Dulat S on 1/4/24.
//

#include "main.h"
int SDLCALL eventFilter(__attribute__((unused))void *userdata, SDL_Event *event) {
    if (event->type == SDL_FINGERDOWN || event->type == SDL_FINGERUP || event->type == SDL_FINGERMOTION) {
        return 0;  // Ignore touch events
    }
    return 1;  // Process all other events as usual
}

void keyboard(AppState *appState) {
    SDL_Event event;
    bool quit = false;
    uint8_t *code = &appState->state->memory[appState->state->mm.peripheralControl.startAddress + 4];
    uint8_t *value = &appState->state->memory[appState->state->mm.peripheralControl.startAddress + 5];


    if (SDL_Init(SDL_INIT_EVENTS) < 0) {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
        return ;
    }
    // In your main function or initialization code
    SDL_SetEventFilter(eventFilter, NULL);
    SDL_Window* window = SDL_CreateWindow("SDL Keyboard Event Example",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              640, 480, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
        SDL_Quit();
        return;
    }

    while (!quit) {
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                quit = true;
                printf("quit\n");
                break;
            }
            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                const int evValue = (event.type == SDL_KEYDOWN) ? 1 : 0;
                if (event.key.repeat == 0) { // Ignore key repeat events
                    printf("Key event: sdlCode %d cpuCode %01x value %d\n", event.key.keysym.scancode, sdlToCpuCode(event.key.keysym.scancode), evValue);
                    *code = sdlToCpuCode(event.key.keysym.scancode);
                    *value = evValue;
                    if(evValue == 1) push_interrupt(appState->state->i_queue, 0x01);
                }
            }
        }
    }

    // Destroy the window
    SDL_DestroyWindow(window);

    // Check for errors
    if (SDL_GetError() != NULL) {
        fprintf(stderr, "Error in destroying window: %s\n", SDL_GetError());
    }

    // Quit SDL
    SDL_Quit();
}
