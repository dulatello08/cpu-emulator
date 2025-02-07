//
// Created by Dulat S on 2/6/25.
//
#include "main.h"

/**
 * read8 - Reads an 8-bit value from memory.
 *
 * @param state   Pointer to the CPU state.
 * @param address The memory address to read from.
 * @return        The 8-bit value read (or 0 if the address is invalid).
 */
uint8_t read8(CPUState* state, uint32_t address) {
    uint8_t* ptr = get_memory_ptr(state, address, false);
    if (ptr == NULL) {
        // Address not allocated. In a real emulator, you might signal an error.
        return 0;
    }
    return *ptr;
}

/**
 * read16 - Reads a 16-bit big-endian value from memory.
 *
 * @param state   Pointer to the CPU state.
 * @param address The starting memory address.
 * @return        The 16-bit value read.
 */
uint16_t read16(CPUState* state, uint32_t address) {
    uint8_t* ptr = get_memory_ptr(state, address, false);
    if (ptr == NULL) {
        return 0;
    }
    // Combine two bytes into a 16-bit value (big-endian).
    return ((uint16_t)ptr[0] << 8) | ptr[1];
}

/**
 * read32 - Reads a 32-bit big-endian value from memory.
 *
 * @param state   Pointer to the CPU state.
 * @param address The starting memory address.
 * @return        The 32-bit value read.
 */
uint32_t read32(CPUState* state, uint32_t address) {
    uint8_t* ptr = get_memory_ptr(state, address, false);
    if (ptr == NULL) {
        return 0;
    }
    return ((uint32_t)ptr[0] << 24) |
           ((uint32_t)ptr[1] << 16) |
           ((uint32_t)ptr[2] << 8)  |
           ptr[3];
}

/**
 * write8 - Writes an 8-bit value to memory.
 *
 * @param state   Pointer to the CPU state.
 * @param address The memory address to write to.
 * @param value   The 8-bit value to write.
 */
void write8(CPUState* state, uint32_t address, uint8_t value) {
    uint8_t* ptr = get_memory_ptr(state, address, true);
    if (ptr) {
        *ptr = value;
    }
}

/**
 * write16 - Writes a 16-bit big-endian value to memory.
 *
 * @param state   Pointer to the CPU state.
 * @param address The starting memory address.
 * @param value   The 16-bit value to write.
 */
void write16(CPUState* state, uint32_t address, uint16_t value) {
    uint8_t* ptr = get_memory_ptr(state, address, true);
    if (ptr) {
        ptr[0] = (value >> 8) & 0xFF;
        ptr[1] = value & 0xFF;
    }
}

/**
 * write32 - Writes a 32-bit big-endian value to memory.
 *
 * @param state   Pointer to the CPU state.
 * @param address The starting memory address.
 * @param value   The 32-bit value to write.
 */
void write32(CPUState* state, uint32_t address, uint32_t value) {
    uint8_t* ptr = get_memory_ptr(state, address, true);
    if (ptr) {
        ptr[0] = (value >> 24) & 0xFF;
        ptr[1] = (value >> 16) & 0xFF;
        ptr[2] = (value >> 8)  & 0xFF;
        ptr[3] = value & 0xFF;
    }
}

void mov(CPUState *state,
         uint8_t rd,
         uint8_t rn,
         uint8_t rn1,
         uint16_t immediate,
         uint32_t normAddress,
         uint32_t offset,
         uint8_t specifier)
{
    switch (specifier) {
        // 0x00: Move immediate into rd (any width, 16 or 8).
        // Syntax: mov 1, #0x1234
        case 0x00:
            state->reg[rd] = immediate;
            break;

        // 0x01: Move rd to rn (full width, 16).
        // Syntax: mov 1, 2  ->  state->reg[rn] = state->reg[rd]
        case 0x01:
            state->reg[rn] = state->reg[rd];
            break;

        // 0x02: Move 8-bit value from memory (normAddressing) to rd.L.
        // Syntax: mov 1.L, [0x2000]
        case 0x02: {
            uint8_t regIndex = rd & 0x3F;  // clear bits 7 and 6 for .L access
            uint8_t value = read8(state, normAddress);
            // Replace low 8 bits; keep the high 8 bits.
            state->reg[regIndex] = (state->reg[regIndex] & 0xFF00) | value;
            break;
        }

        // 0x03: Move 8-bit value from memory (normAddressing) to rd.H.
        // Syntax: mov 1.H, [0x2001]
        case 0x03: {
            uint8_t regIndex = rd & 0x3F;  // clear bits for .H access
            uint8_t value = read8(state, normAddress);
            // Replace high 8 bits; keep the low 8 bits.
            state->reg[regIndex] = (state->reg[regIndex] & 0x00FF) | (value << 8);
            break;
        }

        // 0x04: Move 16-bit value from memory (normAddressing) to rd.
        // Syntax: mov 1, [0x3000]
        case 0x04:
            state->reg[rd] = read16(state, normAddress);
            break;

        // 0x05: Move 32-bit value from memory (normAddressing) into rd and rn1.
        // Upper 16 bits go to rd, lower 16 bits go to rn1.
        // Syntax: mov 1, 2, [0x4000]
        case 0x05: {
            uint32_t value = read32(state, normAddress);
            state->reg[rd]  = (uint16_t)(value >> 16);
            state->reg[rn1] = (uint16_t)(value & 0xFFFF);
            break;
        }

        // 0x06: Move 8-bit value from rd.L to memory (normAddressing).
        // Syntax: mov [0x5000], 1.L
        case 0x06: {
            uint8_t regIndex = rd & 0x3F;
            uint8_t value = state->reg[regIndex] & 0x00FF;
            printf("%c", value);
            fflush(stdout);
            write8(state, normAddress, value);
            break;
        }

        // 0x07: Move 8-bit value from rd.H to memory (normAddressing).
        // Syntax: mov [0x5001], 1.H
        case 0x07: {
            uint8_t regIndex = rd & 0x3F;
            uint8_t value = (state->reg[regIndex] >> 8) & 0x00FF;
            write8(state, normAddress, value);
            break;
        }

        // 0x08: Move 16-bit value from rd to memory (normAddressing).
        // Syntax: mov [0x6000], 1
        case 0x08:
            write16(state, normAddress, state->reg[rd]);
            break;

        // 0x09: Move 32-bit value from rd and rn1 to memory (normAddressing).
        // rd provides the upper 16 bits, rn1 the lower 16 bits.
        // Syntax: mov [0x7000], 1, 2
        case 0x09: {
            uint32_t value = ((uint32_t)state->reg[rd] << 16) | state->reg[rn1];
            write32(state, normAddress, value);
            break;
        }

        // 0x0A: Move 8-bit value from memory (rn + offset) to rd.L.
        // Syntax: mov 1.L, [3 + 0x10]
        case 0x0A: {
            uint32_t effectiveAddress = state->reg[rn] + offset;
            uint8_t value = read8(state, effectiveAddress);
            uint8_t regIndex = rd & 0x3F;
            state->reg[regIndex] = (state->reg[regIndex] & 0xFF00) | value;
            break;
        }

        // 0x0B: Move 8-bit value from memory (rn + offset) to rd.H.
        // Syntax: mov 1.H, [3 + 0x10]
        case 0x0B: {
            uint32_t effectiveAddress = state->reg[rn] + offset;
            uint8_t value = read8(state, effectiveAddress);
            uint8_t regIndex = rd & 0x3F;
            state->reg[regIndex] = (state->reg[regIndex] & 0x00FF) | (value << 8);
            break;
        }

        // 0x0C: Move 16-bit value from memory (rn + offset) to rd.
        // Syntax: mov 1, [3 + 0x20]
        case 0x0C: {
            uint32_t effectiveAddress = state->reg[rn] + offset;
            state->reg[rd] = read16(state, effectiveAddress);
            break;
        }

        // 0x0D: Move 32-bit value from memory (rn + offset) to rd and rn1.
        // Syntax: mov 1, 2, [3 + 0x30]
        case 0x0D: {
            uint32_t effectiveAddress = state->reg[rn] + offset;
            uint32_t value = read32(state, effectiveAddress);
            state->reg[rd]  = (uint16_t)(value >> 16);
            state->reg[rn1] = (uint16_t)(value & 0xFFFF);
            break;
        }

        // 0x0E: Move 8-bit value from rd.L to memory (rn + offset).
        // Syntax: mov [3 + 0x40], 1.L
        case 0x0E: {
            uint32_t effectiveAddress = state->reg[rn] + offset;
            uint8_t regIndex = rd & 0x3F;
            uint8_t value = state->reg[regIndex] & 0x00FF;
            write8(state, effectiveAddress, value);
            break;
        }

        // 0x0F: Move 8-bit value from rd.H to memory (rn + offset).
        // Syntax: mov [3 + 0x40], 1.H
        case 0x0F: {
            uint32_t effectiveAddress = state->reg[rn] + offset;
            uint8_t regIndex = rd & 0x3F;
            uint8_t value = (state->reg[regIndex] >> 8) & 0x00FF;
            write8(state, effectiveAddress, value);
            break;
        }

        // 0x10: Move 16-bit value from rd to memory (rn + offset).
        // Syntax: mov [3 + 0x50], 1
        case 0x10: {
            uint32_t effectiveAddress = state->reg[rn] + offset;
            write16(state, effectiveAddress, state->reg[rd]);
            break;
        }

        // 0x11: Move 32-bit value from rd and rn1 to memory (rn + offset).
        // Syntax: mov [3 + 0x60], 1, 2
        case 0x11: {
            uint32_t effectiveAddress = state->reg[rn] + offset;
            uint32_t value = ((uint32_t)state->reg[rd] << 16) | state->reg[rn1];
            write32(state, effectiveAddress, value);
            break;
        }

        default:
            // Unknown specifier: No operation (or error handling could be added here).
            break;
    }
}
