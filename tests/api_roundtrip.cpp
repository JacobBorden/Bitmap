#include <iostream>
#include <vector>
#include <cstdint>
#include <cassert>
#include <cstring> // For std::memcpy
#include <string>  // For std::to_string (not directly used in snippet but good practice)
#include <span>    // For std::span

// Adjust include paths based on how the project is built/structured.
// These paths assume 'tests' is a sibling to 'include' and 'src'.
#include "../include/bitmap.hpp"
#include "../src/bitmapfile/bitmap_file.h" // For BITMAPFILEHEADER

// Helper function to compare bitmaps
bool compare_bitmaps(const BmpTool::Bitmap& bmp1, const BmpTool::Bitmap& bmp2) {
    if (bmp1.w != bmp2.w || bmp1.h != bmp2.h || bmp1.bpp != bmp2.bpp || bmp1.data.size() != bmp2.data.size()) {
        std::cerr << "Bitmap dimensions or data size mismatch." << std::endl;
        std::cerr << "BMP1: W=" << bmp1.w << ", H=" << bmp1.h << ", BPP=" << bmp1.bpp << ", DataSize=" << bmp1.data.size() << std::endl;
        std::cerr << "BMP2: W=" << bmp2.w << ", H=" << bmp2.h << ", BPP=" << bmp2.bpp << ", DataSize=" << bmp2.data.size() << std::endl;
        return false;
    }
    for (size_t i = 0; i < bmp1.data.size(); ++i) {
        if (bmp1.data[i] != bmp2.data[i]) {
            std::cerr << "Bitmap data mismatch at index " << i << ": "
                      << static_cast<int>(bmp1.data[i]) << " != " << static_cast<int>(bmp2.data[i]) << std::endl;
            // For verbose debugging, print more context around mismatch
            // size_t CONTEXT = 5;
            // size_t start = (i > CONTEXT) ? (i - CONTEXT) : 0;
            // size_t end = std::min(bmp1.data.size(), i + CONTEXT);
            // std::cerr << "Context BMP1: ";
            // for(size_t j=start; j<end; ++j) std::cerr << static_cast<int>(bmp1.data[j]) << " ";
            // std::cerr << std::endl;
            // std::cerr << "Context BMP2: ";
            // for(size_t j=start; j<end; ++j) std::cerr << static_cast<int>(bmp2.data[j]) << " ";
            // std::cerr << std::endl;
            return false;
        }
    }
    return true;
}

int main() {
    std::cout << "Starting Bitmap API roundtrip test..." << std::endl;

    // Create an initial BmpTool::Bitmap object
    BmpTool::Bitmap original_bmp;
    original_bmp.w = 2;
    original_bmp.h = 2;
    original_bmp.bpp = 32; // API uses RGBA
    original_bmp.data = {
        255, 0,   0,   255, // Pixel (0,0) Red
        0,   255, 0,   255, // Pixel (1,0) Green
        0,   0,   255, 255, // Pixel (0,1) Blue
        255, 255, 0,   128  // Pixel (1,1) Yellow, semi-transparent
    };
    std::cout << "Original bitmap created (2x2, 32bpp)." << std::endl;

    // --- First Save ---
    std::vector<uint8_t> saved_buffer1;
    // Estimate size: BITMAPFILEHEADER + BITMAPINFOHEADER + pixel data (W*H*4 for 32bpp)
    // Exact header size for standard BMP is 14 + 40 = 54 bytes.
    size_t estimated_size1 = 54 + original_bmp.w * original_bmp.h * 4;
    saved_buffer1.resize(estimated_size1);
    std::cout << "Buffer 1 resized to " << estimated_size1 << " for first save." << std::endl;

    auto save_result1 = BmpTool::save(original_bmp, std::span<uint8_t>(saved_buffer1));
    assert(save_result1.isSuccess());
    if (!save_result1.isSuccess()) {
        std::cerr << "TEST FAILED: First save failed with error: " << static_cast<int>(save_result1.error()) << std::endl;
        return 1;
    }
    std::cout << "First save successful." << std::endl;

    // --- First Load ---
    // Read bfSize from the saved buffer to correctly size the span for load
    BITMAPFILEHEADER fh1; // From src/bitmapfile/bitmap_file.h
    assert(saved_buffer1.size() >= sizeof(BITMAPFILEHEADER));
    std::memcpy(&fh1, saved_buffer1.data(), sizeof(BITMAPFILEHEADER));
    assert(fh1.bfType == 0x4D42); // Check magic 'BM'
    assert(fh1.bfSize <= saved_buffer1.size()); // Ensure bfSize is within buffer
    std::span<const uint8_t> load_span1(saved_buffer1.data(), fh1.bfSize);
    std::cout << "Load span 1 created with size " << fh1.bfSize << " from buffer." << std::endl;

    auto load_result1 = BmpTool::load(load_span1);
    assert(load_result1.isSuccess());
    if (!load_result1.isSuccess()) {
        std::cerr << "TEST FAILED: First load failed with error: " << static_cast<int>(load_result1.error()) << std::endl;
        return 1;
    }
    BmpTool::Bitmap loaded_bmp1 = load_result1.value();
    std::cout << "First load successful." << std::endl;

    // --- Compare Original and First Loaded Bitmap ---
    std::cout << "Comparing original_bmp and loaded_bmp1..." << std::endl;
    assert(compare_bitmaps(original_bmp, loaded_bmp1));
    if (!compare_bitmaps(original_bmp, loaded_bmp1)) {
         std::cerr << "TEST FAILED: original_bmp and loaded_bmp1 are not identical." << std::endl;
         return 1;
    }
    std::cout << "Original and first loaded bitmaps are identical." << std::endl;

    // --- Second Save (from loaded_bmp1) ---
    std::vector<uint8_t> saved_buffer2;
    size_t estimated_size2 = 54 + loaded_bmp1.w * loaded_bmp1.h * 4;
    saved_buffer2.resize(estimated_size2);
    std::cout << "Buffer 2 resized to " << estimated_size2 << " for second save." << std::endl;

    auto save_result2 = BmpTool::save(loaded_bmp1, std::span<uint8_t>(saved_buffer2));
    assert(save_result2.isSuccess());
     if (!save_result2.isSuccess()) {
        std::cerr << "TEST FAILED: Second save failed with error: " << static_cast<int>(save_result2.error()) << std::endl;
        return 1;
    }
    std::cout << "Second save successful." << std::endl;

    // --- Second Load ---
    BITMAPFILEHEADER fh2;
    assert(saved_buffer2.size() >= sizeof(BITMAPFILEHEADER));
    std::memcpy(&fh2, saved_buffer2.data(), sizeof(BITMAPFILEHEADER));
    assert(fh2.bfType == 0x4D42);
    assert(fh2.bfSize <= saved_buffer2.size());
    std::span<const uint8_t> load_span2(saved_buffer2.data(), fh2.bfSize);
    std::cout << "Load span 2 created with size " << fh2.bfSize << " from buffer." << std::endl;
    
    auto load_result2 = BmpTool::load(load_span2);
    assert(load_result2.isSuccess());
    if (!load_result2.isSuccess()) {
        std::cerr << "TEST FAILED: Second load failed with error: " << static_cast<int>(load_result2.error()) << std::endl;
        return 1;
    }
    BmpTool::Bitmap loaded_bmp2 = load_result2.value();
    std::cout << "Second load successful." << std::endl;

    // --- Compare First Loaded and Second Loaded Bitmap ---
    std::cout << "Comparing loaded_bmp1 and loaded_bmp2..." << std::endl;
    assert(compare_bitmaps(loaded_bmp1, loaded_bmp2));
    if (!compare_bitmaps(loaded_bmp1, loaded_bmp2)) {
         std::cerr << "TEST FAILED: loaded_bmp1 and loaded_bmp2 are not identical." << std::endl;
         return 1;
    }
    std::cout << "First and second loaded bitmaps are identical." << std::endl;

    std::cout << "Bitmap API roundtrip test PASSED!" << std::endl;
    return 0;
}
