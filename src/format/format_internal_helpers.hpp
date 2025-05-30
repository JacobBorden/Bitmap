#pragma once

#include <cstddef> // For size_t
// Include bitmap.h for the definition of ::Pixel
#include "../bitmap/bitmap.h" 

// Helper function to convert an array of ::Pixel (BGRA order) to an array of uint8_t (RGBA order)
void internal_swizzle_bgra_to_rgba_simd(const ::Pixel* src_bgra_pixels, uint8_t* dest_rgba_data, size_t num_pixels);

// Helper function to convert an array of uint8_t (RGBA order) to an array of ::Pixel (BGRA order)
void internal_swizzle_rgba_to_bgra_simd(const uint8_t* src_rgba_data, ::Pixel* dest_bgra_pixels, size_t num_pixels);
