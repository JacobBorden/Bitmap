#include <cstddef>
#include <cstdint>
#include <vector>
#include "../src/bitmap/bitmap.h" // For ::Pixel
#include "../src/format/format_internal_helpers.hpp" // For internal_swizzle_bgra_to_rgba_simd

const size_t MAX_FUZZ_PIXELS = 1024 * 1024;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    if (Size < sizeof(::Pixel)) { // Need at least one ::Pixel
        return 0;
    }
    // Ensure Size is a multiple of sizeof(::Pixel)
    // If not, we can't safely cast Data to ::Pixel* for the full Size.
    // The function expects a certain number of full Pixel structs.
    if (Size % sizeof(::Pixel) != 0) {
        // Or alternatively, could calculate num_pixels based on floor(Size / sizeof(::Pixel))
        // and only pass that many. For stricter fuzzing, returning if not aligned might be intended.
        return 0; 
    }

    size_t num_pixels = Size / sizeof(::Pixel);
    if (num_pixels == 0) { // Should be caught by Size < sizeof(::Pixel) already
        return 0;
    }
    if (num_pixels > MAX_FUZZ_PIXELS) {
        num_pixels = MAX_FUZZ_PIXELS;
    }
    
    std::vector<uint8_t> rgba_output(num_pixels * 4);
    // Data is cast to const ::Pixel*. The function will read num_pixels from this array.
    // This is safe because num_pixels was derived from Size / sizeof(::Pixel).
    // If num_pixels was capped, it processes a sub-segment.
    internal_swizzle_bgra_to_rgba_simd(reinterpret_cast<const ::Pixel*>(Data), rgba_output.data(), num_pixels);
    
    return 0;
}
