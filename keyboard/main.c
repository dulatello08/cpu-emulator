//
// Created by Dulat S on 1/4/24.
//

#include "main.h"

void keyboard(__attribute__((unused)) uint8_t* code, __attribute__((unused)) uint8_t* value) {
    SDL_Event event;
    bool quit = false;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
        return ;
    }

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
            } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                const int evValue = (event.type == SDL_KEYDOWN) ? 1 : 0;
                if (event.key.repeat == 0) { // Ignore key repeat events
                    printf("Key event: sdlCode %d cpuCode %01x value %d\n", event.key.keysym.scancode, sdlToCpuCode(event.key.keysym.scancode), evValue);
                    *code = sdlToCpuCode(event.key.keysym.scancode);
                    *value = evValue;
                }
            }
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
}
