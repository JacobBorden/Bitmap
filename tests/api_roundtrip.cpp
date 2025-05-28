#include <vector>
#include <cstdint>
#include <cstring> // For std::memcpy
#include <iostream> // Keep for potential temporary debug, but not for final assertions
#include <span>    // For std::span

#include <gtest/gtest.h> // Added

// Adjust include paths based on how the project is built/structured.
// These paths assume 'tests' is a sibling to 'include' and 'src'.
#include "../include/bitmap.hpp"
#include "../src/bitmapfile/bitmap_file.h" // For BITMAPFILEHEADER

// Removed main function and compare_bitmaps helper

TEST(ApiRoundtripTest, LoadSaveLoadIdempotency) {
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
    // No std::cout needed here, GTest will announce test start.

    // --- First Save ---
    std::vector<uint8_t> saved_buffer1;
    // Estimate size: BITMAPFILEHEADER + BITMAPINFOHEADER + pixel data (W*H*4 for 32bpp)
    // Exact header size for standard BMP is 14 + 40 = 54 bytes.
    size_t estimated_size1 = 54 + original_bmp.w * original_bmp.h * 4;
    saved_buffer1.resize(estimated_size1);

    auto save_result1 = BmpTool::save(original_bmp, std::span<uint8_t>(saved_buffer1));
    ASSERT_TRUE(save_result1.isSuccess());
    // No need to check error() == Ok here due to Result<void,E> specialization

    // --- First Load ---
    BITMAPFILEHEADER fh1; // From src/bitmapfile/bitmap_file.h
    ASSERT_GE(saved_buffer1.size(), sizeof(BITMAPFILEHEADER));
    std::memcpy(&fh1, saved_buffer1.data(), sizeof(BITMAPFILEHEADER));
    ASSERT_EQ(fh1.bfType, 0x4D42); // Check magic 'BM'
    ASSERT_LE(fh1.bfSize, saved_buffer1.size()); // Ensure bfSize is within buffer
    // Ensure bfSize is sensible
    ASSERT_GE(fh1.bfSize, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)); // Min possible size with info header


    std::span<const uint8_t> load_span1(saved_buffer1.data(), fh1.bfSize);

    auto load_result1 = BmpTool::load(load_span1);
    ASSERT_TRUE(load_result1.isSuccess()) << "First load failed with error: " << static_cast<int>(load_result1.error());
    BmpTool::Bitmap loaded_bmp1 = load_result1.value();

    // --- Compare Original and First Loaded Bitmap ---
    ASSERT_EQ(loaded_bmp1.w, original_bmp.w);
    ASSERT_EQ(loaded_bmp1.h, original_bmp.h);
    ASSERT_EQ(loaded_bmp1.bpp, original_bmp.bpp);
    ASSERT_EQ(loaded_bmp1.data.size(), original_bmp.data.size());
    // For vector data comparison, ASSERT_EQ works directly as std::vector::operator== is defined.
    ASSERT_EQ(loaded_bmp1.data, original_bmp.data);


    // --- Second Save (from loaded_bmp1) ---
    std::vector<uint8_t> saved_buffer2;
    size_t estimated_size2 = 54 + loaded_bmp1.w * loaded_bmp1.h * 4;
    saved_buffer2.resize(estimated_size2);

    auto save_result2 = BmpTool::save(loaded_bmp1, std::span<uint8_t>(saved_buffer2));
    ASSERT_TRUE(save_result2.isSuccess());

    // --- Second Load ---
    BITMAPFILEHEADER fh2;
    ASSERT_GE(saved_buffer2.size(), sizeof(BITMAPFILEHEADER));
    std::memcpy(&fh2, saved_buffer2.data(), sizeof(BITMAPFILEHEADER));
    ASSERT_EQ(fh2.bfType, 0x4D42);
    ASSERT_LE(fh2.bfSize, saved_buffer2.size());
    ASSERT_GE(fh2.bfSize, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)); // Min possible size

    std::span<const uint8_t> load_span2(saved_buffer2.data(), fh2.bfSize);
    
    auto load_result2 = BmpTool::load(load_span2);
    ASSERT_TRUE(load_result2.isSuccess()) << "Second load failed with error: " << static_cast<int>(load_result2.error());
    BmpTool::Bitmap loaded_bmp2 = load_result2.value();

    // --- Compare First Loaded and Second Loaded Bitmap ---
    ASSERT_EQ(loaded_bmp2.w, loaded_bmp1.w);
    ASSERT_EQ(loaded_bmp2.h, loaded_bmp1.h);
    ASSERT_EQ(loaded_bmp2.bpp, loaded_bmp1.bpp);
    ASSERT_EQ(loaded_bmp2.data, loaded_bmp1.data);
}
