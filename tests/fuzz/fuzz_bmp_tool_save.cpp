#include "../../include/bitmap.hpp" // For BmpTool::Bitmap, BmpTool::save, BmpTool::BitmapError
#include <cstdint>
#include <vector>
#include <algorithm> // For std::min
#include <cstring>   // For std::memcpy
#include <stdexcept> // For std::bad_alloc

// Helper to consume data from the fuzzer input
template <typename T>
T Consume(const uint8_t** data_ptr, size_t* size_ptr) {
    if (*size_ptr < sizeof(T)) {
        return T{}; // Return default value if not enough data
    }
    T value;
    std::memcpy(&value, *data_ptr, sizeof(T));
    *data_ptr += sizeof(T);
    *size_ptr -= sizeof(T);
    return value;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    const uint8_t* original_Data_ptr = Data; 
    size_t original_Size = Size;          

    // 1. Construct BmpTool::Bitmap from fuzzer data
    BmpTool::Bitmap bmp;
    bmp.w = Consume<uint32_t>(&Data, &Size) % 1024; 
    bmp.h = Consume<uint32_t>(&Data, &Size) % 1024; 
    
    uint8_t bpp_choice = Consume<uint8_t>(&Data, &Size);
    if (bpp_choice % 3 == 0) { 
        bmp.bpp = 32;
    } else if (bpp_choice % 3 == 1) { 
        bmp.bpp = 24;
    } else { 
        bmp.bpp = Consume<uint8_t>(&Data, &Size);
    }

    size_t bytes_per_pixel = 0;
    if (bmp.bpp > 0) { 
        bytes_per_pixel = (bmp.bpp + 7) / 8;
    } else { 
        bmp.bpp = 24; // Default to a valid bpp
        bytes_per_pixel = 3;
    }
    
    size_t expected_pixel_data_size = static_cast<size_t>(bmp.w) * bmp.h * bytes_per_pixel;
    
    const size_t MAX_PIXEL_DATA_ALLOC = 1024 * 1024 * 4; // 4MB
    size_t pixel_data_to_read = 0;

    if (bmp.w > 0 && bmp.h > 0) { 
        pixel_data_to_read = expected_pixel_data_size;
        if (pixel_data_to_read > MAX_PIXEL_DATA_ALLOC) {
            pixel_data_to_read = MAX_PIXEL_DATA_ALLOC; 
        }
        if (pixel_data_to_read > Size) { 
            pixel_data_to_read = Size;
        }
        if (pixel_data_to_read > 0) {
            try {
                bmp.data.assign(Data, Data + pixel_data_to_read);
                Data += pixel_data_to_read;
                Size -= pixel_data_to_read;
            } catch (const std::bad_alloc&) {
                bmp.data.clear(); // Clear if assign fails
            }
        } else {
            bmp.data.clear(); 
        }
    } else {
        bmp.data.clear(); 
    }

    if (bmp.data.empty() && bmp.w > 0 && bmp.h > 0 && expected_pixel_data_size > 0) {
        size_t fill_size = std::min(expected_pixel_data_size, (size_t)256); 
        fill_size = std::min(fill_size, MAX_PIXEL_DATA_ALLOC); 
        if (fill_size > 0) { 
            try {
                bmp.data.resize(fill_size, 0xAB);
            } catch (const std::bad_alloc&) {
                bmp.data.clear();
            }
        }
    }

    // 2. Prepare output buffer
    uint16_t output_buffer_fuzz_val = Consume<uint16_t>(&Data, &Size); 
    const size_t MAX_OUTPUT_BUFFER_ALLOC = 1024 * 1024 * 8; // 8MB
    
    size_t final_output_buffer_size = 54 + (output_buffer_fuzz_val % (MAX_OUTPUT_BUFFER_ALLOC - 53)); 
    final_output_buffer_size = std::min(final_output_buffer_size, MAX_OUTPUT_BUFFER_ALLOC);
    final_output_buffer_size = std::max((size_t)54, final_output_buffer_size);

    std::vector<uint8_t> out_buffer;
    try {
        out_buffer.resize(final_output_buffer_size);
        if (Size > 0) {
            std::memcpy(out_buffer.data(), Data, std::min(Size, out_buffer.size()));
        } else {
            // Try to use some portion of original data if current 'Data' pointer is past 'original_Data_ptr'
            // and there's unconsumed original data.
            size_t consumed_for_bmp_etc = Data - original_Data_ptr;
            if (original_Size > consumed_for_bmp_etc) {
                 std::memcpy(out_buffer.data(), original_Data_ptr + consumed_for_bmp_etc, std::min(original_Size - consumed_for_bmp_etc, out_buffer.size()));
            }
        }
    } catch (const std::bad_alloc&) {
        try {
            out_buffer.resize(54); 
        } catch (const std::bad_alloc&) {
            return 0; 
        }
    }
    
    // 3. Call BmpTool::save
    [[maybe_unused]] auto result = BmpTool::save(bmp, std::span<uint8_t>(out_buffer.data(), out_buffer.size()));

    return 0; 
}
