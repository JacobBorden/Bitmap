#include "../../include/bitmap.hpp" // For BmpTool::load and BmpTool::BitmapError
#include <cstdint>
#include <vector> // Required by span
#include <span>   // For std::span

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    // Pass arbitrary length memory span to BmpTool::load.
    // Handles 0 bytes, partial headers (<54), valid headers, and arbitrary payloads.
    std::span<const uint8_t> bmp_data(Data, Size);
    auto result = BmpTool::load(bmp_data);

    // If parsing succeeds, verify basic invariants to catch logic errors
    if (result.isSuccess()) {
        const auto& bmp = result.value();
        if (bmp.w > 0 && bmp.h > 0) {
            // Buffer size should match computed dimensions
            size_t bytes_per_pixel = bmp.bpp / 8;
            if (bmp.data.size() != static_cast<size_t>(bmp.w) * bmp.h * bytes_per_pixel) {
                return -1;
            }
        }
    }

    return 0; // Essential for libFuzzer to continue.
}
