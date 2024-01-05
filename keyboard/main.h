//
// Created by Dulat S on 1/4/24.
//

#ifndef MAIN_H
#define MAIN_H

#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>

uint8_t sdlToCpuCode(int sdlCode);
void keyboard(__attribute__((unused)) uint8_t* code, __attribute__((unused)) uint8_t* value);

#endif //MAIN_H
