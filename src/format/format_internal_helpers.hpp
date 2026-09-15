#pragma once

#include <cstddef> // For size_t
// Include bitmap.h for the definition of ::Pixel from the core library,
// assuming the swizzle functions operate on ::Pixel.
// The path might need to be relative to where this header is included from,
// or we assume include paths are set up for "bitmap/bitmap.h" or similar.
// Given its previous usage in tests/test_swizzle.cpp as "../src/bitmap/bitmap.h" (if that's what it was)
// and in src/format/bitmap.cpp as "../../src/bitmap/bitmap.h"
// For a header in src/format/, to reach src/bitmap/bitmap.h, it would be "../bitmap/bitmap.h"
#include "../bitmap/bitmap.h"

namespace BmpTool {

// These functions are defined in src/format/bitmap.cpp
void internal_swizzle_bgra_to_rgba_simd(const ::Pixel* src_bgra_pixels, uint8_t* dest_rgba_data, size_t num_pixels);
void internal_swizzle_rgba_to_bgra_simd(const uint8_t* src_rgba_data, ::Pixel* dest_bgra_pixels, size_t num_pixels);

} // namespace BmpTool
