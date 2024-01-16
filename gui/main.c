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

int renderText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y) {
    // Create a surface from the string
    SDL_Color textColor = {255, 255, 255, 255}; // White color
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, text, textColor);
    if (!textSurface) {
        fprintf(stderr, "Failed to create text surface: %s\n", TTF_GetError());
        return 0;
    }

    // Create a texture from the surface
    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    if (!textTexture) {
        fprintf(stderr, "Failed to create text texture: %s\n", SDL_GetError());
        return 0;
    }

    // Get the texture size
    int textWidth = 0, textHeight = 0;
    SDL_QueryTexture(textTexture, NULL, NULL, &textWidth, &textHeight);

    // Set the rendering position
    SDL_Rect renderQuad = {x, y, textWidth, textHeight};

    // Render the text
    SDL_RenderCopy(renderer, textTexture, NULL, &renderQuad);

    // Clean up
    SDL_DestroyTexture(textTexture);
    return textHeight;
}


void handleInput(SDL_Renderer *renderer, TTF_Font *font) {
    static int currentX = 0;
    static int currentY = 0;
    int lineHeight = 24; // Assuming 24 is the height of each line of text

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
            if (strncmp(buffer, "D(", 2) == 0) {
                // Process the input and display it
                char *start = strchr(buffer, '"');
                if (start && start[1] != '\0') {
                    char *end = strchr(start + 1, '"');
                    if (end) {
                        *end = '\0'; // Null-terminate the string
                        char *nextLine = strchr(start, '\n');

                        if (nextLine) {
                            *nextLine = '\0'; // Replace newline with null-terminator
                            currentY += renderText(renderer, font, start + 1, currentX, currentY) + lineHeight;
                            currentY += lineHeight; // Move down for the next text line
                            currentX = 0; // Reset X position
                        } else {
                            int textWidth = renderText(renderer, font, start + 1, currentX, currentY);
                            currentX += textWidth; // Move right for the next text
                        }

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

    // Initialize TTF library
    if (TTF_Init() < 0) {
        fprintf(stderr, "Could not initialize SDL_ttf: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("/Users/dulat/Documents/Games/X-Plane 12/Resources/fonts/DejaVuSans.ttf", 24); // Adjust font path and size
    if (!font) {
        fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_SetEventFilter(eventFilter, NULL);
    SDL_Window* window = SDL_CreateWindow("SDL Keyboard Event Example",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              640, 480, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (window == NULL) {
        fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    while (!quit) {
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                quit = true;
                break;
            }
            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                const int evValue = (event.type == SDL_KEYDOWN) ? 1 : 0;
                if (event.key.repeat == 0) { // Ignore key repeat events
                    printf("Key event: sdlCode %d value %d\n", event.key.keysym.scancode, evValue);
                }
            }
        }
        handleInput(renderer, font);
    }
    TTF_CloseFont(font);
    TTF_Quit();
    // Destroy the window
    SDL_DestroyWindow(window);

    // Quit SDL
    SDL_Quit();
    fprintf(stderr, "quit");
    return 0;
}
