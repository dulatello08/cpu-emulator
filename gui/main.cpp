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

void update_display(char display[LCD_WIDTH][LCD_HEIGHT], SDL_Renderer *renderer, TTF_Font *font);

void sigUsrHandler(int signal, signal_handler_data_t *signal_handler_data) {
    static signal_handler_data_t *saved = nullptr;

    if (saved == nullptr)
        saved = signal_handler_data;
    if (signal == SIGUSR1) {
        // Enqueue an update request
        if (saved->queueCount < MAX_QUEUE_SIZE) {
            saved->updateQueue[saved->queueTail] = 1; // 1 represents an update request
            saved->queueTail = (saved->queueTail + 1) % MAX_QUEUE_SIZE;
            ++saved->queueCount;
        }
    }
}

void update_display(char display[LCD_WIDTH][LCD_HEIGHT], SDL_Renderer *renderer, TTF_Font *font) {
    SDL_RenderClear(renderer);
    SDL_Color textColor = {255, 255, 255, 255}; // White text color
    int lineHeight = TTF_FontHeight(font);
    int currentY = 0;

    char sampleChar = 'A';
    char sampleStr[2] = {sampleChar, '\0'};
    SDL_Surface* sampleSurface = TTF_RenderText_Blended(font, sampleStr, textColor);
    int fixedCharWidth = sampleSurface->w + 1;
    SDL_FreeSurface(sampleSurface);

    for (int j = 0; j < LCD_HEIGHT; j++) {
        for (int i = 0; i < LCD_WIDTH; i++) {
            char character = display[i][j];
            if (character == '\0') {
                character = '*'; // Replace null character with '*'
            }
            //printf("Rendering character: %c at (%d, %d)\n", character, i, j);

            char str[2] = {character, '\0'}; // Convert char to string for rendering
            SDL_Surface* textSurface = TTF_RenderText_Blended(font, str, textColor);
            if (textSurface == nullptr) {
                fprintf(stderr, "Unable to create text surface: %s\n", TTF_GetError());
                return;
            }

            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            if (textTexture == nullptr) {
                fprintf(stderr, "Unable to create texture from surface: %s\n", SDL_GetError());
                SDL_FreeSurface(textSurface);
                return;
            }

            SDL_Rect textRect = {i * fixedCharWidth, currentY, fixedCharWidth, lineHeight};
            SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);

            SDL_DestroyTexture(textTexture);
            SDL_FreeSurface(textSurface);
        }
        currentY += lineHeight; // Move to the next line
    }

    SDL_RenderPresent(renderer); // Update the screen with rendering performed
}

int main() {
    signal(SIGUSR1, (void (*)(int)) sigUsrHandler);
    signal_handler_data_t signal_handler_data;
    signal_handler_data.queueHead = 0;
    signal_handler_data.queueTail = 0;
    signal_handler_data.queueCount = 0;
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

    TTF_Font *font = TTF_OpenFont("Menlo-Regular.ttf", 24);
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

    memcpy(signal_handler_data.display, shared_memory->display, sizeof(signal_handler_data.display));
    signal_handler_data.font = font;
    signal_handler_data.renderer = renderer;

    sigUsrHandler(SIGUSR2, &signal_handler_data);

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
                    shared_memory->keyboard_o[0] = sdlToCpuCode(event.key.keysym.scancode);
                    shared_memory->keyboard_o[1] = evValue;
                    if(evValue) push_interrupt(&shared_memory->i_queue, 0x01);
                }
            }
        }
        if (signal_handler_data.queueCount > 0) {
            update_display(signal_handler_data.display, signal_handler_data.renderer, signal_handler_data.font);
            signal_handler_data.queueHead = (signal_handler_data.queueHead + 1) % MAX_QUEUE_SIZE;
            --signal_handler_data.queueCount;
        }
        memcpy(signal_handler_data.display, shared_memory->display, sizeof(signal_handler_data.display));
    }
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyWindow(window);
    SDL_Quit();
    cerr << "quit" << '\n';
    return 0;
}