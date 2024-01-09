//
// Created by Dulat S on 1/4/24.
//

#include "main.h"

void keyboard(AppState *appState) {
    SDL_Event event;
    bool quit = false;
    uint8_t *code = &appState->state->memory[appState->state->mm.peripheralControl.startAddress + 4];
    uint8_t *value = &appState->state->memory[appState->state->mm.peripheralControl.startAddress + 5];


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
                printf("quit");
                break;
            } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
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

    // Assuming 'window' is your SDL_Window*

    // Get the surface associated with the window
    SDL_Surface* surface = SDL_GetWindowSurface(window);

    // Fill the surface with a color, for example, black
    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));

    // Update the window surface
    SDL_UpdateWindowSurface(window);

    // Optional: Delay for a brief moment
    SDL_Delay(100); // Delay in milliseconds, e.g., 100ms

    // Destroy the window
    SDL_DestroyWindow(window);

    // Check for errors
    if (SDL_GetError() != NULL) {
        fprintf(stderr, "Error in destroying window: %s\n", SDL_GetError());
    }

    // Quit SDL
    SDL_Quit();keyboard
}
