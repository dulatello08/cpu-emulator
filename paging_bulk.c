#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
    #define PLATFORM_X86
    #include <immintrin.h>
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__ARM_NEON)
    #define PLATFORM_ARM
    #include <arm_neon.h>
#endif

// -----------------------------------------------------------------------------
// Byte-Only SIMD memcpy
// -----------------------------------------------------------------------------
/**
 * Optimized helper function to copy 'n' BYTES from 'src' to 'dest'
 * using SIMD instructions. All logic here is in terms of 8-bit bytes.
 */
static inline void *memcpy_simd(void *dest, const void *src, size_t n) {
    // Cast everything to byte pointers
    uint8_t       *dst_ptr  = (uint8_t *)dest;
    const uint8_t *src_ptr  = (const uint8_t *)src;
    size_t         bytes_remaining = n;

    // -------------------------------------------------------------------------
    // 1) Align 'dest' to a 16-byte boundary if possible
    // -------------------------------------------------------------------------
    uintptr_t dst_addr     = (uintptr_t)dst_ptr;
    size_t    misalignment = dst_addr % 16;
    if (misalignment) {
        // How many bytes do we need to write to make dst 16-byte aligned?
        size_t bytes_to_align = 16 - misalignment;
        // Don't exceed what's left to copy
        size_t count = (bytes_to_align < bytes_remaining) ? bytes_to_align : bytes_remaining;

        // Copy these bytes one-by-one
        for (size_t i = 0; i < count; i++) {
            *dst_ptr++ = *src_ptr++;
        }
        bytes_remaining -= count;
    }

    // -------------------------------------------------------------------------
    // 2) Copy in large chunks (SIMD)
    // -------------------------------------------------------------------------
#if defined(PLATFORM_X86)
    // AVX2: 32 bytes at a time
    while (bytes_remaining >= 32) {
        __m256i data256 = _mm256_loadu_si256((const __m256i*)src_ptr);
        _mm256_storeu_si256((__m256i*)dst_ptr, data256);
        dst_ptr        += 32;
        src_ptr        += 32;
        bytes_remaining -= 32;
    }
    // SSE2: 16 bytes at a time
    while (bytes_remaining >= 16) {
        __m128i data128 = _mm_loadu_si128((const __m128i*)src_ptr);
        _mm_storeu_si128((__m128i*)dst_ptr, data128);
        dst_ptr        += 16;
        src_ptr        += 16;
        bytes_remaining -= 16;
    }

#elif defined(PLATFORM_ARM)
    // NEON: 16 bytes at a time
    while (bytes_remaining >= 16) {
        // Load 128 bits (16 bytes)
        uint8x16_t data_neon = vld1q_u8((const uint8_t*)src_ptr);
        vst1q_u8((uint8_t*)dst_ptr, data_neon);
        dst_ptr        += 16;
        src_ptr        += 16;
        bytes_remaining -= 16;
    }
#endif

    // -------------------------------------------------------------------------
    // 3) Copy leftover bytes
    // -------------------------------------------------------------------------
    while (bytes_remaining > 0) {
        *dst_ptr++ = *src_ptr++;
        bytes_remaining--;
    }

    return dest;
}
// -----------------------------------------------------------------------------
// Bulk Copy Function (Now in 8-bit terms)
// -----------------------------------------------------------------------------
/**
 * Copies 'length' BYTES from 'buffer' into CPU memory starting at 'address'.
 * Internally handles page boundaries and uses SIMD for copying.
 */
void bulk_copy_memory(CPUState *state, uint32_t address, const uint8_t *buffer, size_t length) {
    // We will copy from 'buffer[0..length-1]' into CPU memory [address..address+length-1]
    uint32_t end_address   = address + length;
    size_t   buffer_offset = 0;

    while (address < end_address) {
        // Calculate offset within the current page
        uint32_t offset_in_page = address & (PAGE_SIZE - 1);

        // How many BYTES left in this page before hitting boundary?
        uint32_t bytes_in_page   = PAGE_SIZE - offset_in_page;
        uint32_t bytes_remaining = end_address - address;
        uint32_t bytes_to_write  = (bytes_in_page < bytes_remaining) ? bytes_in_page : bytes_remaining;

        // If page boundary is so tight we can't even write 1 byte, just break
        if (bytes_to_write == 0) {
            break;
        }

        // Get pointer to memory location (allocate page if needed)
        uint8_t *mem_ptr = get_memory_ptr(state, address, true);
        if (!mem_ptr) {
            fprintf(stderr, "Failed to get memory pointer at address 0x%08x\n", address);
            return;
        }

        // Copy chunk using our 8-bit SIMD memcpy
        memcpy_simd(mem_ptr, buffer + buffer_offset, bytes_to_write);

        // Update for next iteration
        address       += bytes_to_write;
        buffer_offset += bytes_to_write;
    }
}
