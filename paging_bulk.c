#include "main.h"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#define PLATFORM_X86
    #include <immintrin.h>
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__ARM_NEON)
#define PLATFORM_ARM
#include <arm_neon.h>
#endif

/**
 * Optimized helper function to copy memory from a source buffer using SIMD instructions.
 * This function handles platform-specific optimizations.
 */
static inline void *memcpy_simd(void *dest, const void *src, size_t n) {
    uint8_t *dst_ptr = (uint8_t *)dest;
    const uint8_t *src_ptr = (const uint8_t *)src;
    size_t bytes_remaining = n * sizeof(uint16_t);

    // Handle unaligned bytes at the start
    uintptr_t dst_addr = (uintptr_t)dst_ptr;
    size_t misalignment = dst_addr % 16;

    if (misalignment) {
        size_t bytes_to_align = 16 - misalignment;
        size_t words_to_align = bytes_to_align / sizeof(uint16_t);
        size_t count = words_to_align < n ? words_to_align : n;

        for (size_t i = 0; i < count; ++i) {
            *((uint16_t *)dst_ptr) = *((const uint16_t *)src_ptr);
            dst_ptr += sizeof(uint16_t);
            src_ptr += sizeof(uint16_t);
            bytes_remaining -= sizeof(uint16_t);
            n--;
        }
    }

#if defined(PLATFORM_X86)
    // Use AVX2 instructions if available
    while (bytes_remaining >= 32) { // 256 bits = 32 bytes
        __m256i data256 = _mm256_loadu_si256((const __m256i *)src_ptr);
        _mm256_storeu_si256((__m256i *)dst_ptr, data256);
        dst_ptr += 32;
        src_ptr += 32;
        bytes_remaining -= 32;
        n -= 16;
    }
    // Use SSE2 instructions for remaining data
    while (bytes_remaining >= 16) {
        __m128i data128 = _mm_loadu_si128((const __m128i *)src_ptr);
        _mm_storeu_si128((__m128i *)dst_ptr, data128);
        dst_ptr += 16;
        src_ptr += 16;
        bytes_remaining -= 16;
        n -= 8;
    }
#elif defined(PLATFORM_ARM)
    // Use NEON instructions
    while (bytes_remaining >= 16) { // 128 bits = 16 bytes
        uint16x8_t data_neon = vld1q_u16((const uint16_t *)src_ptr);
        vst1q_u16((uint16_t *)dst_ptr, data_neon);
        dst_ptr += 16;
        src_ptr += 16;
        bytes_remaining -= 16;
        n -= 8;
    }
#endif

    // Handle any remaining bytes
    while (n > 0) {
        *((uint16_t *)dst_ptr) = *((const uint16_t *)src_ptr);
        dst_ptr += sizeof(uint16_t);
        src_ptr += sizeof(uint16_t);
        n--;
    }

    return dest;
}

/**
 * Optimized function to copy a block of memory from a buffer.
 * This function handles page boundaries and utilizes SIMD instructions.
 */
void bulk_copy_memory(CPUState *state, uint32_t address, const uint16_t *buffer, size_t length) {
    uint32_t end_address = address + length * sizeof(uint16_t);
    size_t buffer_offset = 0;

    while (address < end_address) {
        // Calculate offset within the current page
        uint32_t offset_in_page = address & (PAGE_SIZE - 1);

        // Calculate how many bytes can be written in the current page
        uint32_t bytes_in_page = PAGE_SIZE - offset_in_page;
        uint32_t bytes_remaining = end_address - address;
        uint32_t bytes_to_write = bytes_in_page < bytes_remaining ? bytes_in_page : bytes_remaining;

        // If there aren't enough bytes for a full 16-bit word, handle the remainder and exit loop
        if (bytes_to_write < sizeof(uint16_t)) {
            // Get pointer to the final memory location
            uint8_t *final_mem_ptr = (uint8_t *)get_memory_ptr(state, address, true);
            if (!final_mem_ptr) {
                fprintf(stderr, "Failed to get memory pointer at address 0x%08x\n", address);
                return;
            }
            // Copy any remaining bytes directly
            memcpy(final_mem_ptr, (const uint8_t *)buffer + buffer_offset, bytes_to_write);
            break;
        }

        size_t words_to_write = bytes_to_write / sizeof(uint16_t);

        // Get pointer to memory location, allocate if necessary
        uint16_t *mem_ptr = get_memory_ptr(state, address, true);
        if (!mem_ptr) {
            fprintf(stderr, "Failed to get memory pointer at address 0x%08x\n", address);
            return;
        }

        // Copy memory using the optimized memcpy_simd function
        memcpy_simd(mem_ptr, buffer + buffer_offset, words_to_write);

        // Update address and buffer offset for next iteration
        address += words_to_write * sizeof(uint16_t);
        buffer_offset += words_to_write;
    }
}
