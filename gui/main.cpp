//
// Created by Dulat S on 1/4/24.
//

#include "main.h"

#include <unistd.h>
#include <regex>
#include <string>
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

using namespace std;

void update_display(char display[LCD_WIDTH][LCD_HEIGHT], SDL_Renderer *renderer, TTF_Font *font) {
    SDL_Color textColor = {255, 255, 255, 255}; // White color for text
    int currentX = 0;
    int currentY = 0;
    int lineHeight = 24; // Adjust as needed

    for (int y = 0; y < LCD_HEIGHT; ++y) {
        for (int x = 0; x < LCD_WIDTH; ++x) {
            char charToRender[2] = {display[x][y], '\0'}; // Convert to string for rendering

            SDL_Surface* textSurface = TTF_RenderText_Blended(font, charToRender, textColor);
            if (textSurface == nullptr) {
                std::cerr << "Unable to create text surface: " << TTF_GetError() << '\n';
                continue;
            }

            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            if (textTexture == nullptr) {
                std::cerr << "Unable to create texture from surface: " << SDL_GetError() << '\n';
                SDL_FreeSurface(textSurface);
                continue;
            }

            SDL_Rect textRect = {currentX, currentY, textSurface->w, textSurface->h};
            SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);

            SDL_DestroyTexture(textTexture);
            SDL_FreeSurface(textSurface);

            currentX += 18;
            if (currentX >= LCD_WIDTH * 18) {
                currentX = 0;
                currentY += lineHeight;
            }
        }
    }
}

int main() {
    SDL_Event event;
    bool quit = false;

    if (SDL_Init(SDL_INIT_EVENTS) < 0) {
        cerr << "Could not initialize SDL: " << SDL_GetError() << '\n';
        return 1;
    }

    if (TTF_Init() < 0) {
        cerr << "Could not initialize SDL_ttf: " << TTF_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("/Users/dulat/Downloads/Menlo-Regular-01.ttf", 24);
    if (!font) {
        cerr << "Failed to load font: " << TTF_GetError() << '\n';
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("NeoCore emulator GUI",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          640, 480, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (window == nullptr) {
        cerr << "Could not create window: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    const char *memname = "emulator_gui_shm";
    const size_t size = sizeof(gui_process_shm_t);

    // Create and open a shared memory object in the parent process
    int memFd = shm_open(memname, O_RDWR, 0);
    if (memFd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    auto *shared_memory = (gui_process_shm_t*) mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, memFd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
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
                if (event.key.repeat == 0) {
                    printf("Key event: cpu code %d value %d\n", sdlToCpuCode(event.key.keysym.scancode), evValue);
                }
            }
        }
        update_display(shared_memory->display, renderer, font);
    }
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyWindow(window);
    SDL_Quit();
    cerr << "quit" << '\n';
    return 0;
}