//
// Created by Dulat S on 1/14/24.
//
//
// Created by Dulat S on 1/4/24.
//

#include <SDL.h>
#include <stdbool.h>

int SDLCALL eventFilter(__attribute__((unused))void *userdata, SDL_Event *event) {
    if (event->type == SDL_FINGERDOWN || event->type == SDL_FINGERUP || event->type == SDL_FINGERMOTION) {
        return 0;  // Ignore touch events
    }
    return 1;  // Process all other events as usual
}


int main() {
    SDL_Event event;
    bool quit = false;


    if (SDL_Init(SDL_INIT_EVENTS) < 0) {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
        return 1;
    }
    // In your main function or initialization code
    SDL_SetEventFilter(eventFilter, NULL);
    SDL_Window* window = SDL_CreateWindow("SDL Keyboard Event Example",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              640, 480, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    while (!quit) {
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                quit = true;
                printf("quit\n");
                break;
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
    return 0;
}
