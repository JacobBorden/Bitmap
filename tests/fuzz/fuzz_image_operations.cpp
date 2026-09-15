#include "../../src/bitmap/bitmap.h" // For image manipulation functions
#include "../../src/bitmapfile/bitmap_file.h" // For Bitmap::File
#include <cstdint>
#include <vector>
#include <string>
#include <cstring>   // For std::memcpy
#include <algorithm> // For std::min, std::max
#include <stdexcept> // For std::bad_alloc (and potentially other std::exceptions)

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
float ConsumeFloat(const uint8_t** data_ptr, size_t* size_ptr, float min_val = 0.0f, float max_val = 2.0f) { // Default values for min_val and max_val
    if (*size_ptr == 0) return (min_val + max_val) / 2.0f; // Default if no data
    uint8_t byte_val = Consume<uint8_t>(data_ptr, size_ptr);
    if (min_val == max_val) return min_val;
    return min_val + (static_cast<float>(byte_val) / 255.0f) * (max_val - min_val);
}

// Helper to consume an integer within a range
int ConsumeInt(const uint8_t** data_ptr, size_t* size_ptr, int min_val = 0, int max_val = 10) { // Default values for min_val and max_val
    if (*size_ptr == 0) return min_val; // Default if no data
    uint8_t byte_val = Consume<uint8_t>(data_ptr, size_ptr);
    if (max_val == min_val) return min_val;
    int range = max_val - min_val + 1;
    if (range <= 0) range = 1; 
    return min_val + (byte_val % range);
}


extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    if (Size < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 1) { 
        return 0;
    }

    Bitmap::File bmp_file; 

    if (Size < sizeof(BITMAPFILEHEADER)) return 0;
    std::memcpy(&bmp_file.bitmapFileHeader, Data, sizeof(BITMAPFILEHEADER));
    Data += sizeof(BITMAPFILEHEADER);
    Size -= sizeof(BITMAPFILEHEADER);

    if (Size < sizeof(BITMAPINFOHEADER)) return 0;
    std::memcpy(&bmp_file.bitmapInfoHeader, Data, sizeof(BITMAPINFOHEADER));
    Data += sizeof(BITMAPINFOHEADER);
    Size -= sizeof(BITMAPINFOHEADER);
    
    if (bmp_file.bitmapInfoHeader.biWidth < 0) bmp_file.bitmapInfoHeader.biWidth = 0;
    bmp_file.bitmapInfoHeader.biWidth = std::min(bmp_file.bitmapInfoHeader.biWidth, (LONG)1024); 
    if (bmp_file.bitmapInfoHeader.biHeight < 0) bmp_file.bitmapInfoHeader.biHeight = 0;
    bmp_file.bitmapInfoHeader.biHeight = std::min(bmp_file.bitmapInfoHeader.biHeight, (LONG)1024); 
    
    short valid_bpp[] = {8, 16, 24, 32};
    bool bpp_is_valid = false;
    for(short b : valid_bpp) { if(bmp_file.bitmapInfoHeader.biBitCount == b) {bpp_is_valid = true; break;}}
    if (!bpp_is_valid) bmp_file.bitmapInfoHeader.biBitCount = 24; 

    if (Size > 0) {
        try {
            const size_t MAX_BITMAP_DATA_ALLOC = 1024 * 1024 * 4; // 4MB limit
            size_t data_to_assign = std::min(Size, MAX_BITMAP_DATA_ALLOC);
            bmp_file.bitmapData.assign(Data, Data + data_to_assign);
            Data += data_to_assign; 
            Size -= data_to_assign;
        } catch (const std::bad_alloc&) {
            bmp_file.bitmapData.clear(); 
        }
    }
    
    bmp_file.SetValid(); 

    uint8_t operation_choice = Consume<uint8_t>(&Data, &Size); 

    Bitmap::File result_bmp_file; 
    
    switch (operation_choice % 24) { 
        case 0: result_bmp_file = ShrinkImage(bmp_file, ConsumeInt(&Data, &Size, 1, 8)); break;
        case 1: result_bmp_file = RotateImageCounterClockwise(bmp_file); break;
        case 2: result_bmp_file = RotateImageClockwise(bmp_file); break;
        case 3: result_bmp_file = MirrorImage(bmp_file); break;
        case 4: result_bmp_file = FlipImage(bmp_file); break;
        case 5: result_bmp_file = GreyscaleImage(bmp_file); break;
        case 6: result_bmp_file = ChangeImageBrightness(bmp_file, ConsumeFloat(&Data, &Size, 0.1f, 3.0f)); break;
        case 7: result_bmp_file = ChangeImageContrast(bmp_file, ConsumeFloat(&Data, &Size, 0.1f, 3.0f)); break;
        case 8: result_bmp_file = ChangeImageSaturation(bmp_file, ConsumeFloat(&Data, &Size, 0.0f, 3.0f)); break;
        case 9: result_bmp_file = ChangeImageSaturationBlue(bmp_file, ConsumeFloat(&Data, &Size, 0.0f, 3.0f)); break;
        case 10: result_bmp_file = ChangeImageSaturationGreen(bmp_file, ConsumeFloat(&Data, &Size, 0.0f, 3.0f)); break;
        case 11: result_bmp_file = ChangeImageSaturationRed(bmp_file, ConsumeFloat(&Data, &Size, 0.0f, 3.0f)); break;
        case 12: result_bmp_file = ChangeImageSaturationMagenta(bmp_file, ConsumeFloat(&Data, &Size, 0.0f, 3.0f)); break;
        case 13: result_bmp_file = ChangeImageSaturationYellow(bmp_file, ConsumeFloat(&Data, &Size, 0.0f, 3.0f)); break;
        case 14: result_bmp_file = ChangeImageSaturationCyan(bmp_file, ConsumeFloat(&Data, &Size, 0.0f, 3.0f)); break;
        case 15: result_bmp_file = ChangeImageLuminanceBlue(bmp_file, ConsumeFloat(&Data, &Size, 0.0f, 2.0f)); break;
        case 16: result_bmp_file = ChangeImageLuminanceGreen(bmp_file, ConsumeFloat(&Data, &Size, 0.0f, 2.0f)); break;
        case 17: result_bmp_file = ChangeImageLuminanceRed(bmp_file, ConsumeFloat(&Data, &Size, 0.0f, 2.0f)); break;
        case 18: result_bmp_file = ChangeImageLuminanceMagenta(bmp_file, ConsumeFloat(&Data, &Size, 0.0f, 2.0f)); break;
        case 19: result_bmp_file = ChangeImageLuminanceYellow(bmp_file, ConsumeFloat(&Data, &Size, 0.0f, 2.0f)); break;
        case 20: result_bmp_file = ChangeImageLuminanceCyan(bmp_file, ConsumeFloat(&Data, &Size, 0.0f, 2.0f)); break;
        case 21: result_bmp_file = InvertImageColors(bmp_file); break;
        case 22: result_bmp_file = ApplySepiaTone(bmp_file); break;
        case 23: result_bmp_file = ApplyBoxBlur(bmp_file, ConsumeInt(&Data, &Size, 0, 3)); break; 
    }
    
    return 0;
}
