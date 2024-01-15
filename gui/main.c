//
// Created by Dulat S on 1/4/24.
//

#include "main.h"

#include <unistd.h>

int SDLCALL eventFilter(__attribute__((unused))void *userdata, SDL_Event *event) {
    if (event->type == SDL_FINGERDOWN || event->type == SDL_FINGERUP || event->type == SDL_FINGERMOTION) {
        return 0;  // Ignore touch events
    }
    return 1;  // Process all other events as usual
}

void renderText(SDL_Renderer *renderer, const char *text) {
    // Initialize TTF library
    if (TTF_Init() < 0) {
        fprintf(stderr, "Could not initialize SDL_ttf: %s\n", TTF_GetError());
        return;
    }

    // Load a font
    TTF_Font *font = TTF_OpenFont("/Users/dulat/Documents/Games/X-Plane 12/Resources/fonts/Menlo-Regular.ttf", 24); // Adjust font path and size
    if (!font) {
        fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
        TTF_Quit();
        return;
    }

    // Create a surface from the string
    SDL_Color textColor = {255, 255, 255, 255}; // White color, fully opaque
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, text, textColor);
    if (!textSurface) {
        fprintf(stderr, "Failed to create text surface: %s\n", TTF_GetError());
        TTF_CloseFont(font);
        TTF_Quit();
        return;
    }

    // Create a texture from the surface
    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    if (!textTexture) {
        fprintf(stderr, "Failed to create text texture: %s\n", SDL_GetError());
        TTF_CloseFont(font);
        TTF_Quit();
        return;
    }

    // Get the texture size
    int textWidth = 0, textHeight = 0;
    SDL_QueryTexture(textTexture, NULL, NULL, &textWidth, &textHeight);
    SDL_Rect renderQuad = {0, 0, textWidth, textHeight};

    // Render the text
    SDL_RenderCopy(renderer, textTexture, NULL, &renderQuad);

    // Clean up
    SDL_DestroyTexture(textTexture);
    TTF_CloseFont(font);
    TTF_Quit();
}


void handleInput(SDL_Renderer *renderer) {
    char buffer[1024];
    fd_set set;
    struct timeval timeout;

    // Initialize the file descriptor set.
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    // Initialize the timeout data structure.
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;

    // Check for input to read.
    int result = select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout);
    if (result > 0) {
        // Read input from stdin
        if (fgets(buffer, sizeof(buffer), stdin)) {
            if (strncmp(buffer, "DISPLAY(", 8) == 0) {
                // Process the input and display it
                char *start = strchr(buffer, '"');
                if (start && start[1] != '\0') {
                    char *end = strchr(start + 1, '"');
                    if (end) {
                        *end = '\0'; // Null-terminate the string

                        // Clear screen
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
                        SDL_RenderClear(renderer);

                        // Here you would render the text to the screen
                        // For simplicity, let's just print it to stdout
                        printf("Rendering: %s\n", start + 1);
                        renderText(renderer, start + 1);

                        // Update screen
                        SDL_RenderPresent(renderer);
                    }
                }
            }
        }
    }
}


int main(void) {
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
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
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
            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                const int evValue = (event.type == SDL_KEYDOWN) ? 1 : 0;
                if (event.key.repeat == 0) { // Ignore key repeat events
                    printf("Key event: sdlCode %d value %d\n", event.key.keysym.scancode, evValue);
                }
            }
        }
        handleInput(renderer);
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
