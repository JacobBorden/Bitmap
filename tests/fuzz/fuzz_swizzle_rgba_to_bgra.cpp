#include <cstddef>
#include <cstdint>
#include <vector>
#include "../src/bitmap/bitmap.h" // For ::Pixel
#include "../src/format/format_internal_helpers.hpp" // For internal_swizzle_rgba_to_bgra_simd

const size_t MAX_FUZZ_PIXELS = 1024 * 1024;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    if (Size < 4) { // Need at least one RGBA pixel (4 bytes)
        return 0;
    }
    // Ensure Size is a multiple of 4 for RGBA pixels
    if (Size % 4 != 0) {
        // Similar to the BGRA->RGBA fuzzer, could truncate or return.
        // Returning if not perfectly divisible ensures strict input format.
        return 0;
    }

    size_t num_pixels = Size / 4;
    if (num_pixels == 0) { // Should be caught by Size < 4 already
        return 0;
    }
    if (num_pixels > MAX_FUZZ_PIXELS) {
        num_pixels = MAX_FUZZ_PIXELS;
    }

    std::vector<::Pixel> bgra_output(num_pixels);
    // Data is uint8_t* source. internal_swizzle_rgba_to_bgra_simd expects this.
    // It will read num_pixels * 4 bytes from Data.
    // This is safe as num_pixels was derived from Size / 4.
    // If num_pixels was capped, it processes a sub-segment.
    internal_swizzle_rgba_to_bgra_simd(Data, bgra_output.data(), num_pixels);

    return 0;
}
