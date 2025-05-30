#include <cstddef>
#include <cstdint>
#include <vector>
#include "../src/bitmap/bitmap.h" // For ::Pixel and internal_convert_bgr_to_bgra_simd

// Limit max pixels to prevent excessive memory allocation during fuzzing.
const size_t MAX_FUZZ_PIXELS = 1024 * 1024; // Approx 4MB for BGRA, 3MB for BGR

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    if (Size < 3) { // Need at least one BGR pixel
        return 0;
    }

    size_t num_pixels = Size / 3;
    if (num_pixels == 0) {
        return 0;
    }
    if (num_pixels > MAX_FUZZ_PIXELS) {
        num_pixels = MAX_FUZZ_PIXELS;
        // Adjust Size to match the truncated num_pixels for the source data,
        // though internal_convert_bgr_to_bgra_simd only reads num_pixels * 3 bytes.
        // No, Data pointer is const, and Size is the original size.
        // The function will just process fewer pixels from the original Data buffer.
    }

    std::vector<::Pixel> output_pixels(num_pixels);
    // The source data (Data) is used directly.
    // internal_convert_bgr_to_bgra_simd will read num_pixels * 3 bytes from Data.
    // This is safe because num_pixels was derived from Size/3.
    // If num_pixels was capped by MAX_FUZZ_PIXELS, it reads a sub-segment of the original data.
    internal_convert_bgr_to_bgra_simd(Data, output_pixels.data(), num_pixels);

    return 0; // Non-zero return values are reserved for future use.
}
