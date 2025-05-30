#include <vector>
#include <cstdint> // For uint8_t
#include <string>  // For std::to_string
#include "gtest/gtest.h"
// For Pixel struct and internal_convert_bgr_to_bgra_simd declaration
#include "../src/bitmap/bitmap.h"
#include "../src/format/format_internal_helpers.hpp" // For BGRA <-> RGBA swizzle helpers
// #include "../src/simd_utils.hpp" // Not strictly needed here as func is in .cpp

// --- Tests for BGR -> BGRA swizzling (from internal_convert_bgr_to_bgra_simd) ---

// Test case for a single pixel conversion BGR -> BGRA
TEST(SwizzleBGRtoBGRATest, SinglePixel) {
    uint8_t bgr_input[] = {0x01, 0x02, 0x03}; // B, G, R
    ::Pixel output_pixel_array[1];
    internal_convert_bgr_to_bgra_simd(bgr_input, output_pixel_array, 1);
    EXPECT_EQ(output_pixel_array[0].blue, 0x01);
    EXPECT_EQ(output_pixel_array[0].green, 0x02);
    EXPECT_EQ(output_pixel_array[0].red, 0x03);
    EXPECT_EQ(output_pixel_array[0].alpha, 255);
}

// Helper function for various sizes test for BGR -> BGRA
void check_bgr_to_bgra_conversion(size_t num_pixels) {
    SCOPED_TRACE("num_pixels (BGRtoBGRA): " + std::to_string(num_pixels));
    if (num_pixels == 0) {
        internal_convert_bgr_to_bgra_simd(nullptr, nullptr, 0);
        SUCCEED();
        return;
    }
    std::vector<uint8_t> bgr_input(num_pixels * 3);
    std::vector<::Pixel> output_pixels(num_pixels);
    for (size_t i = 0; i < num_pixels; ++i) {
        bgr_input[i*3+0] = static_cast<uint8_t>(((i*3+1)%250) + 1); // Blue
        bgr_input[i*3+1] = static_cast<uint8_t>(((i*3+2)%250) + 1); // Green
        bgr_input[i*3+2] = static_cast<uint8_t>(((i*3+3)%250) + 1); // Red
    }
    internal_convert_bgr_to_bgra_simd(bgr_input.data(), output_pixels.data(), num_pixels);
    for (size_t j = 0; j < num_pixels; ++j) {
        ASSERT_EQ(output_pixels[j].blue, bgr_input[j*3+0]);
        ASSERT_EQ(output_pixels[j].green, bgr_input[j*3+1]);
        ASSERT_EQ(output_pixels[j].red, bgr_input[j*3+2]);
        ASSERT_EQ(output_pixels[j].alpha, 255);
    }
}

TEST(SwizzleBGRtoBGRATest, VariousSizes) {
    std::vector<size_t> sizes_to_test = {0, 1, 3, 4, 7, 8, 15, 16, 31, 32, 63, 64, 100};
    for (size_t size : sizes_to_test) {
        check_bgr_to_bgra_conversion(size);
    }
}

TEST(SwizzleBGRtoBGRATest, AllZeros) {
    const size_t num_pixels = 32;
    std::vector<uint8_t> bgr_input(num_pixels * 3, 0);
    std::vector<::Pixel> output_pixels(num_pixels);
    internal_convert_bgr_to_bgra_simd(bgr_input.data(), output_pixels.data(), num_pixels);
    for (size_t j = 0; j < num_pixels; ++j) {
        EXPECT_EQ(output_pixels[j].blue, 0);
        EXPECT_EQ(output_pixels[j].green, 0);
        EXPECT_EQ(output_pixels[j].red, 0);
        EXPECT_EQ(output_pixels[j].alpha, 255);
    }
}

TEST(SwizzleBGRtoBGRATest, AllMaxRGB) {
    const size_t num_pixels = 32;
    const uint8_t max_val = 254;
    std::vector<uint8_t> bgr_input(num_pixels * 3, max_val);
    std::vector<::Pixel> output_pixels(num_pixels);
    internal_convert_bgr_to_bgra_simd(bgr_input.data(), output_pixels.data(), num_pixels);
    for (size_t j = 0; j < num_pixels; ++j) {
        EXPECT_EQ(output_pixels[j].blue, max_val);
        EXPECT_EQ(output_pixels[j].green, max_val);
        EXPECT_EQ(output_pixels[j].red, max_val);
        EXPECT_EQ(output_pixels[j].alpha, 255);
    }
}

// --- Tests for BGRA <-> RGBA swizzling (from format_internal_helpers.hpp) ---

// Helper function for BGRA -> RGBA tests
void check_bgra_to_rgba_conversion(size_t num_pixels) {
    SCOPED_TRACE("num_pixels (BGRAtoRGBA): " + std::to_string(num_pixels));
    if (num_pixels == 0) {
        BmpTool::internal_swizzle_bgra_to_rgba_simd(nullptr, nullptr, 0);
        SUCCEED();
        return;
    }
    std::vector<::Pixel> bgra_input(num_pixels);
    for (size_t j = 0; j < num_pixels; ++j) {
        bgra_input[j].blue  = static_cast<uint8_t>(((j * 4 + 0) % 250) + 1);
        bgra_input[j].green = static_cast<uint8_t>(((j * 4 + 1) % 250) + 1);
        bgra_input[j].red   = static_cast<uint8_t>(((j * 4 + 2) % 250) + 1);
        bgra_input[j].alpha = static_cast<uint8_t>(((j * 4 + 3) % 250) + 1);
    }
    std::vector<uint8_t> rgba_output(num_pixels * 4);

    BmpTool::internal_swizzle_bgra_to_rgba_simd(bgra_input.data(), rgba_output.data(), num_pixels);

    for (size_t j = 0; j < num_pixels; ++j) {
        ASSERT_EQ(rgba_output[j*4+0], bgra_input[j].red);   // R
        ASSERT_EQ(rgba_output[j*4+1], bgra_input[j].green); // G
        ASSERT_EQ(rgba_output[j*4+2], bgra_input[j].blue);  // B
        ASSERT_EQ(rgba_output[j*4+3], bgra_input[j].alpha); // A
    }
}

TEST(SwizzleBGRARGATest, BGRAtoRGBAVariousSizes) {
    std::vector<size_t> sizes_to_test = {0, 1, 3, 4, 7, 8, 15, 16, 31, 32, 63, 64, 100};
    for (size_t size : sizes_to_test) {
        check_bgra_to_rgba_conversion(size);
    }
}

// Helper function for RGBA -> BGRA tests
void check_rgba_to_bgra_conversion(size_t num_pixels) {
    SCOPED_TRACE("num_pixels (RGBAtoBGRA): " + std::to_string(num_pixels));
    if (num_pixels == 0) {
        BmpTool::internal_swizzle_rgba_to_bgra_simd(nullptr, nullptr, 0);
        SUCCEED();
        return;
    }
    std::vector<uint8_t> rgba_input(num_pixels * 4);
    for (size_t j = 0; j < num_pixels; ++j) {
        rgba_input[j*4+0] = static_cast<uint8_t>(((j * 4 + 0) % 250) + 1); // R
        rgba_input[j*4+1] = static_cast<uint8_t>(((j * 4 + 1) % 250) + 1); // G
        rgba_input[j*4+2] = static_cast<uint8_t>(((j * 4 + 2) % 250) + 1); // B
        rgba_input[j*4+3] = static_cast<uint8_t>(((j * 4 + 3) % 250) + 1); // A
    }
    std::vector<::Pixel> bgra_output(num_pixels);

    BmpTool::internal_swizzle_rgba_to_bgra_simd(rgba_input.data(), bgra_output.data(), num_pixels);

    for (size_t j = 0; j < num_pixels; ++j) {
        ASSERT_EQ(bgra_output[j].red,   rgba_input[j*4+0]); // R
        ASSERT_EQ(bgra_output[j].green, rgba_input[j*4+1]); // G
        ASSERT_EQ(bgra_output[j].blue,  rgba_input[j*4+2]); // B
        ASSERT_EQ(bgra_output[j].alpha, rgba_input[j*4+3]); // A
    }
}

TEST(SwizzleBGRARGATest, RGBAtoBGRAVariousSizes) {
    std::vector<size_t> sizes_to_test = {0, 1, 3, 4, 7, 8, 15, 16, 31, 32, 63, 64, 100};
    for (size_t size : sizes_to_test) {
        check_rgba_to_bgra_conversion(size);
    }
}
