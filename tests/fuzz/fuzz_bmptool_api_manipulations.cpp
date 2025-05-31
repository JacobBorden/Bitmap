#include "../../include/bitmap.hpp" // For BmpTool API
#include <cstdint>
#include <vector>
#include <string>
#include <cstring>   // For std::memcpy
#include <algorithm> // For std::min
#include <stdexcept> // For std::bad_alloc

// Helper to consume data from the fuzzer input
template <typename T>
T Consume(const uint8_t** data_ptr, size_t* size_ptr) {
    if (*size_ptr < sizeof(T)) {
        return T{};
    }
    T value;
    std::memcpy(&value, *data_ptr, sizeof(T));
    *data_ptr += sizeof(T);
    *size_ptr -= sizeof(T);
    return value;
}

// Helper to consume a float value (scaled from a byte)
float ConsumeFloat(const uint8_t** data_ptr, size_t* size_ptr, float min_val = 0.0f, float max_val = 2.0f) {
    if (*size_ptr == 0) return (min_val + max_val) / 2.0f; // Default if no data
    uint8_t byte_val = Consume<uint8_t>(data_ptr, size_ptr);
    if (min_val == max_val) return min_val;
    float range = max_val - min_val;
    // if (range == 0) return min_val; // Already handled by min_val == max_val
    return min_val + (static_cast<float>(byte_val) / 255.0f) * range;
}

// Helper to consume an integer within a range
int ConsumeInt(const uint8_t** data_ptr, size_t* size_ptr, int min_val = 0, int max_val = 10) {
    if (*size_ptr == 0) return min_val; // Default if no data
    uint8_t byte_val = Consume<uint8_t>(data_ptr, size_ptr);
    if (max_val == min_val) return min_val;
    int range_val = max_val - min_val + 1;
    if (range_val <= 0) range_val = 1; // Avoid modulo by zero or negative range
    return min_val + (byte_val % range_val);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    if (Size < 10) { // Need some minimal data for dimensions and choice
        return 0;
    }

    const uint8_t* current_data_ptr = Data;
    size_t current_size_ptr = Size;

    // 1. Construct BmpTool::Bitmap from fuzzer data
    BmpTool::Bitmap src_bmp;
    // Consume dimensions carefully, ensuring they are not excessively large but also non-zero
    uint16_t w_raw = Consume<uint16_t>(&current_data_ptr, &current_size_ptr);
    uint16_t h_raw = Consume<uint16_t>(&current_data_ptr, &current_size_ptr);

    src_bmp.w = (w_raw % 128) + 1;
    src_bmp.h = (h_raw % 128) + 1;
    src_bmp.bpp = 32;

    size_t pixel_data_size = static_cast<size_t>(src_bmp.w) * src_bmp.h * 4;
    const size_t MAX_ALLOC = 128 * 128 * 4; // Adjusted max to match dimension constraints more closely

    if (pixel_data_size == 0 || pixel_data_size > MAX_ALLOC) { // Should not happen if w,h >0 and within 128
        return 0;
    }

    try {
        src_bmp.data.resize(pixel_data_size);
    } catch (const std::bad_alloc&) {
        return 0; // Cannot allocate memory
    }

    if (current_size_ptr >= pixel_data_size) {
        std::memcpy(src_bmp.data.data(), current_data_ptr, pixel_data_size);
        current_data_ptr += pixel_data_size;
        current_size_ptr -= pixel_data_size;
    } else {
        if (current_size_ptr > 0) {
            std::memcpy(src_bmp.data.data(), current_data_ptr, current_size_ptr);
            // Fill the rest with a repeating pattern of the remaining data to avoid purely zeroed tails
            size_t remaining_fill = pixel_data_size - current_size_ptr;
            uint8_t* fill_ptr = src_bmp.data.data() + current_size_ptr;
            while (remaining_fill > 0) {
                size_t to_copy = std::min(current_size_ptr, remaining_fill);
                if (to_copy == 0) break; // No source data left to copy from, rest will be zero
                std::memcpy(fill_ptr, current_data_ptr, to_copy);
                fill_ptr += to_copy;
                remaining_fill -= to_copy;
            }
        }
        current_size_ptr = 0; // All data consumed for pixels
    }

    // Choose an operation
    uint8_t operation_choice = Consume<uint8_t>(&current_data_ptr, &current_size_ptr);
    BmpTool::Result<BmpTool::Bitmap, BmpTool::BitmapError> result(BmpTool::BitmapError::UnknownError);

    // Ensure parameters for operations are also consumed from the remaining data
    switch (operation_choice % 24) {
        case 0: result = BmpTool::shrink(src_bmp, ConsumeInt(&current_data_ptr, &current_size_ptr, 1, 8)); break;
        case 1: result = BmpTool::rotateCounterClockwise(src_bmp); break;
        case 2: result = BmpTool::rotateClockwise(src_bmp); break;
        case 3: result = BmpTool::mirror(src_bmp); break;
        case 4: result = BmpTool::flip(src_bmp); break;
        case 5: result = BmpTool::greyscale(src_bmp); break;
        case 6: result = BmpTool::changeBrightness(src_bmp, ConsumeFloat(&current_data_ptr, &current_size_ptr, 0.1f, 3.0f)); break;
        case 7: result = BmpTool::changeContrast(src_bmp, ConsumeFloat(&current_data_ptr, &current_size_ptr, 0.1f, 3.0f)); break;
        case 8: result = BmpTool::changeSaturation(src_bmp, ConsumeFloat(&current_data_ptr, &current_size_ptr, 0.0f, 3.0f)); break;
        case 9: result = BmpTool::changeSaturationBlue(src_bmp, ConsumeFloat(&current_data_ptr, &current_size_ptr, 0.0f, 3.0f)); break;
        case 10: result = BmpTool::changeSaturationGreen(src_bmp, ConsumeFloat(&current_data_ptr, &current_size_ptr, 0.0f, 3.0f)); break;
        case 11: result = BmpTool::changeSaturationRed(src_bmp, ConsumeFloat(&current_data_ptr, &current_size_ptr, 0.0f, 3.0f)); break;
        case 12: result = BmpTool::changeSaturationMagenta(src_bmp, ConsumeFloat(&current_data_ptr, &current_size_ptr, 0.0f, 3.0f)); break;
        case 13: result = BmpTool::changeSaturationYellow(src_bmp, ConsumeFloat(&current_data_ptr, &current_size_ptr, 0.0f, 3.0f)); break;
        case 14: result = BmpTool::changeSaturationCyan(src_bmp, ConsumeFloat(&current_data_ptr, &current_size_ptr, 0.0f, 3.0f)); break;
        case 15: result = BmpTool::changeLuminanceBlue(src_bmp, ConsumeFloat(&current_data_ptr, &current_size_ptr, 0.0f, 2.0f)); break;
        case 16: result = BmpTool::changeLuminanceGreen(src_bmp, ConsumeFloat(&current_data_ptr, &current_size_ptr, 0.0f, 2.0f)); break;
        case 17: result = BmpTool::changeLuminanceRed(src_bmp, ConsumeFloat(&current_data_ptr, &current_size_ptr, 0.0f, 2.0f)); break;
        case 18: result = BmpTool::changeLuminanceMagenta(src_bmp, ConsumeFloat(&current_data_ptr, &current_size_ptr, 0.0f, 2.0f)); break;
        case 19: result = BmpTool::changeLuminanceYellow(src_bmp, ConsumeFloat(&current_data_ptr, &current_size_ptr, 0.0f, 2.0f)); break;
        case 20: result = BmpTool::changeLuminanceCyan(src_bmp, ConsumeFloat(&current_data_ptr, &current_size_ptr, 0.0f, 2.0f)); break;
        case 21: result = BmpTool::invertColors(src_bmp); break;
        case 22: result = BmpTool::applySepiaTone(src_bmp); break;
        case 23: result = BmpTool::applyBoxBlur(src_bmp, ConsumeInt(&current_data_ptr, &current_size_ptr, 0, 3)); break;
        default:
            result = BmpTool::greyscale(src_bmp);
            break;
    }

    // Process the result to ensure it's valid or handle errors gracefully.
    // This helps catch issues where operations might produce invalid Bitmap objects
    // that are not necessarily crashes but violate API contracts (e.g., wrong bpp, data size mismatch).
    if (result.isSuccess()) {
        const auto& processed_bmp = result.value();
        if (processed_bmp.w > 0 && processed_bmp.h > 0) {
            if (processed_bmp.bpp != 32) return -1; // Should always be 32
            if (processed_bmp.data.size() != static_cast<size_t>(processed_bmp.w) * processed_bmp.h * 4) {
                 return -1; // Data size mismatch
            }
        } else {
            // If width or height is zero, data should be empty.
            if (!processed_bmp.data.empty()) return -1;
            if (processed_bmp.w == 0 && processed_bmp.h == 0 && processed_bmp.bpp == 0) {
                 // This is a default-constructed state from some operations, treat as valid empty.
            } else if (processed_bmp.bpp != 32 && !(processed_bmp.w == 0 && processed_bmp.h == 0)) {
                 // If not w=0,h=0, bpp should be 32.
                 return -1;
            }
        }
    } // else it's an error, which is fine for fuzzing (API handled it).

    return 0;
}
