//
// Created by Dulat S on 1/4/24.
//

#include "main.h"

#include <unistd.h>
#include <regex>
#include <string>
#include <iostream>

int SDLCALL eventFilter(void *userdata, SDL_Event *event) {
    if (event->type == SDL_FINGERDOWN || event->type == SDL_FINGERUP || event->type == SDL_FINGERMOTION) {
        return 0;  // Ignore touch events
    }
    return 1;  // Process all other events as usual
}

using namespace std;

void handleInput(SDL_Renderer *renderer, TTF_Font *font) {
    static int currentX = 0;
    static int currentY = 0;
    int lineHeight = TTF_FontHeight(font);

    string buffer;
    fd_set set;
    timeval timeout{};

    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;

    if (select(STDIN_FILENO + 1, &set, nullptr, nullptr, &timeout) > 0) {
        getline(cin, buffer);
        if (!buffer.empty()) {
            regex drawCmdRegex(R"(D\((.*)\))");
            regex clearCmdRegex(R"(C\(\))");
            smatch matches;

            if (regex_search(buffer, matches, drawCmdRegex) && matches.size() > 1) {
                string text = matches[1].str();
                regex newlineRegex(R"(\\n)");
                sregex_token_iterator it(text.begin(), text.end(), newlineRegex, -1);
                sregex_token_iterator end;

                SDL_Color textColor = {255, 255, 255, 255};
                while (it != end) {
                    string line = *it++;
                    SDL_Surface* textSurface = TTF_RenderText_Blended(font, line.c_str(), textColor);
                    if (textSurface == nullptr) {
                        cerr << "Unable to create text surface: " << TTF_GetError() << '\n';
                        break;
                    }

                    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                    if (textTexture == nullptr) {
                        cerr << "Unable to create texture from surface: " << SDL_GetError() << '\n';
                        SDL_FreeSurface(textSurface);
                        break;
                    }

                    SDL_Rect textRect = {currentX, currentY, textSurface->w, textSurface->h};
                    SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);

                    SDL_DestroyTexture(textTexture);
                    SDL_FreeSurface(textSurface);

                    currentY += lineHeight;
                    if (currentY >= 480 - lineHeight) {
                        currentY = 0;
                    }
                }
            } else if (regex_match(buffer, clearCmdRegex)) {
                SDL_RenderClear(renderer);
                currentX = currentY = 0;
            }
            SDL_RenderPresent(renderer);
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

    SDL_SetEventFilter(eventFilter, nullptr);
    SDL_Window* window = SDL_CreateWindow("SDL Keyboard Event Example",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          640, 480, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (window == nullptr) {
        cerr << "Could not create window: " << SDL_GetError() << '\n';
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
                if (event.key.repeat == 0) {
                    printf("Key event: cpu code %d value %d\n", sdlToCpuCode(event.key.keysym.scancode), evValue);
                }
            }
        }
        handleInput(renderer, font);
    }
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyWindow(window);
    SDL_Quit();
    cerr << "quit" << '\n';
    return 0;
}