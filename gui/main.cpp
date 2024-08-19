#include "main.h"

#include <unistd.h>
#include <regex>
#include <string>
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <chrono>
#include <thread>

const int FPS_CAP = 60;
bool send_interrupts = true;

using namespace std;

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
    // Catppuccin Mocha colors
    const SDL_Color base = {30, 30, 46, 255};    // Dark background
    const SDL_Color crust = {17, 17, 27, 255};   // Darker shade for borders
    const SDL_Color text = {166, 227, 161, 255}; // Green text, slightly muted for better readability
    const SDL_Color subtext = {116, 199, 236, 255}; // Light blue for accents

    SDL_SetRenderDrawColor(renderer, base.r, base.g, base.b, base.a);
    SDL_RenderClear(renderer);

    int lineHeight = TTF_FontHeight(font);
    int padding = 20;
    int charWidth = 12;
    int displayWidth = LCD_WIDTH * charWidth + padding * 2;
    int displayHeight = LCD_HEIGHT * lineHeight + padding * 2;

    // Create a slightly lighter background for the display area
    SDL_Rect displayArea = {padding, padding, displayWidth, displayHeight};
    SDL_SetRenderDrawColor(renderer, base.r + 10, base.g + 10, base.b + 10, base.a);
    SDL_RenderFillRect(renderer, &displayArea);

    // Add a subtle border
    SDL_SetRenderDrawColor(renderer, crust.r, crust.g, crust.b, crust.a);
    SDL_RenderDrawRect(renderer, &displayArea);

    // LCD-like effect: draw horizontal lines
    SDL_SetRenderDrawColor(renderer, crust.r, crust.g, crust.b, 100); // Semi-transparent
    for (int y = padding; y < padding + displayHeight; y += 2) {
        SDL_RenderDrawLine(renderer, padding, y, padding + displayWidth, y);
    }

    for (int j = 0; j < LCD_HEIGHT; j++) {
        for (int i = 0; i < LCD_WIDTH; i++) {
            char character = display[i][j];
            if (character == '\0') {
                character = ' '; // Replace null character with space
            }

            char str[2] = {character, '\0'};
            SDL_Surface* textSurface = TTF_RenderText_Blended(font, str, text);
            if (textSurface == nullptr) {
                fprintf(stderr, "Unable to create text surface: %s\n", TTF_GetError());
                continue;
            }

            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            if (textTexture == nullptr) {
                fprintf(stderr, "Unable to create texture from surface: %s\n", SDL_GetError());
                SDL_FreeSurface(textSurface);
                continue;
            }

            SDL_Rect textRect = {padding + i * charWidth + charWidth/4, padding + j * lineHeight + lineHeight/4,
                                 textSurface->w, textSurface->h};
            SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);

            SDL_DestroyTexture(textTexture);
            SDL_FreeSurface(textSurface);
        }
    }

    // Display interrupt status
    const char* status = send_interrupts ? "Interrupts: ON" : "Interrupts: OFF";
    SDL_Surface* statusSurface = TTF_RenderText_Blended(font, status, subtext);
    if (statusSurface != nullptr) {
        SDL_Texture* statusTexture = SDL_CreateTextureFromSurface(renderer, statusSurface);
        if (statusTexture != nullptr) {
            SDL_Rect statusRect = {padding, padding + displayHeight + 10, statusSurface->w, statusSurface->h};
            SDL_RenderCopy(renderer, statusTexture, nullptr, &statusRect);
            SDL_DestroyTexture(statusTexture);
        }
        SDL_FreeSurface(statusSurface);
    }

    SDL_RenderPresent(renderer);
}

int main() {
    signal(SIGUSR1, (void (*)(int)) sigUsrHandler);
    signal_handler_data_t signal_handler_data;
    signal_handler_data.queueHead = 0;
    signal_handler_data.queueTail = 0;
    signal_handler_data.queueCount = 0;
    SDL_Event event;
    bool quit = false;
    bool need_redraw = true;  // New flag to track when we need to redraw

    if (SDL_Init(SDL_INIT_EVENTS) < 0) {
        cerr << "Could not initialize SDL: " << SDL_GetError() << '\n';
        return 1;
    }

    if (TTF_Init() < 0) {
        cerr << "Could not initialize SDL_ttf: " << TTF_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("SourceCodePro-Regular.ttf", 16);
    if (!font) {
        cerr << "Failed to load font: " << TTF_GetError() << '\n';
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("NeoCore emulator GUI",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          LCD_WIDTH * 12 + 80, LCD_HEIGHT * TTF_FontHeight(font) + 100,
                                          SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
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
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_i && (event.key.keysym.mod & KMOD_CTRL)) {
                    send_interrupts = !send_interrupts;
                    printf("Interrupts %s\n", send_interrupts ? "enabled" : "disabled");
                    need_redraw = true;  // Mark the display as needing a redraw
                } else if (event.key.repeat == 0) {
                    // Handle other key events
                    printf("Key event: cpu code %d value 1\n", sdlToCpuCode(event.key.keysym.scancode));
                    shared_memory->keyboard_o[0] = sdlToCpuCode(event.key.keysym.scancode);
                    shared_memory->keyboard_o[1] = 1;
                    if (send_interrupts) push_interrupt(&shared_memory->i_queue, 0x01);
                }
            } else if (event.type == SDL_KEYUP) {
                // Handle key up events
                if (event.key.repeat == 0) {
                    printf("Key event: cpu code %d value 0\n", sdlToCpuCode(event.key.keysym.scancode));
                    shared_memory->keyboard_o[0] = sdlToCpuCode(event.key.keysym.scancode);
                    shared_memory->keyboard_o[1] = 0;
                }
            }
        }

        if (signal_handler_data.queueCount > 0 || need_redraw) {
            update_display(signal_handler_data.display, signal_handler_data.renderer, signal_handler_data.font);
            signal_handler_data.queueHead = (signal_handler_data.queueHead + 1) % MAX_QUEUE_SIZE;
            --signal_handler_data.queueCount;
            need_redraw = false;  // Reset the redraw flag after rendering
        }
        memcpy(signal_handler_data.display, shared_memory->display, sizeof(signal_handler_data.display));

        // Cap the frame rate to FPS_CAP using SDL_Delay
        SDL_Delay(1000 / FPS_CAP);
    }

    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyWindow(window);
    SDL_Quit();
    cerr << "quit" << '\n';
    return 0;
}