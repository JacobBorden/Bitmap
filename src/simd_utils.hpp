#pragma once

// For x86/x64 SIMD intrinsics
#if defined(__SSE2__) || defined(__AVX__) || defined(__AVX2__)
#include <immintrin.h> // Includes SSE, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2, AVX, AVX2, FMA, etc.
#endif

// For ARM NEON intrinsics
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#endif

// Helper to check if a pointer is aligned to a certain byte boundary
template <typename T>
inline bool is_aligned(const T* ptr, std::size_t alignment) {
    return reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0;
}
