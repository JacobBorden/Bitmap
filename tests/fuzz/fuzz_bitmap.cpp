#include "../../include/bitmap.hpp" // For BmpTool::load and BmpTool::BitmapError
#include <cstdint>
#include <vector> // Required by span
#include <span>   // For std::span

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    // We are specifically testing 54-byte headers.
    // The fuzzer might provide smaller or larger inputs.
    // We truncate or ignore inputs not of this size to focus the fuzzing.
    if (Size < 54) {
        return 0; // Not enough data for a 54-byte header.
    }

    // Create a span for the 54-byte header.
    std::span<const uint8_t> bmp_data(Data, 54);

    // Call the function to be fuzzed.
    // We don't need to check the result for fuzzing purposes,
    // as ASan/libFuzzer will report crashes or memory errors.
    [[maybe_unused]] auto result = BmpTool::load(bmp_data);

    return 0; // Essential for libFuzzer to continue.
}
