#include <gtest/gtest.h>
#include "../../include/bitmap.hpp" // BmpTool API
#include <vector>
#include <cstdint>
#include <numeric> // For std::iota (not used in this version)
#include <cmath>   // For std::round (used in greyscale for clarity)

// Helper function to get a pixel's RGBA value
// Returns a vector {R, G, B, A}. Asserts if x, y are out of bounds.
std::vector<uint8_t> getPixel(const BmpTool::Bitmap& bmp, uint32_t x, uint32_t y) {
    EXPECT_LT(x, bmp.w);
    EXPECT_LT(y, bmp.h);
    if (x >= bmp.w || y >= bmp.h) return {0,0,0,0}; // Should fail test due to EXPECT_LT

    size_t index = (y * bmp.w + x) * 4; // 4 bytes per pixel (RGBA)
    EXPECT_LT(index + 3, bmp.data.size());
    if (index + 3 >= bmp.data.size()) return {0,0,0,0};

    return {bmp.data[index], bmp.data[index+1], bmp.data[index+2], bmp.data[index+3]};
}


TEST(BmpToolApiManipulationTest, ShrinkValidScaleBy2) {
    BmpTool::Bitmap src;
    src.w = 4; src.h = 4; src.bpp = 32;
    src.data.resize(4 * 4 * 4, 0); // Initialize with black pixels
    // P0 (0,0) = Red
    src.data[0] = 255; src.data[1] = 0;   src.data[2] = 0;   src.data[3] = 255;
    // P1 (1,0) = Green
    src.data[4] = 0;   src.data[5] = 255; src.data[6] = 0;   src.data[7] = 255;
    // P4 (0,1) = Blue
    src.data[16] = 0;  src.data[17] = 0;  src.data[18] = 255; src.data[19] = 255;
    // P5 (1,1) = White
    src.data[20] = 255;src.data[21] = 255;src.data[22] = 255;src.data[23] = 255;

    auto result = BmpTool::shrink(src, 2);
    ASSERT_TRUE(result.isSuccess()) << "Shrink failed: " << static_cast<int>(result.error());
    BmpTool::Bitmap shrunk_bmp = result.value();

    ASSERT_EQ(shrunk_bmp.w, 2);
    ASSERT_EQ(shrunk_bmp.h, 2);
    ASSERT_EQ(shrunk_bmp.bpp, 32);
    ASSERT_EQ(shrunk_bmp.data.size(), 2 * 2 * 4);

    // Pixel (0,0) in shrunk should be average of (0,0),(1,0),(0,1),(1,1) in src
    // R = (255+0+0+255)/4 = 127.5 -> 127 or 128 (core lib uses integer division)
    // G = (0+255+0+255)/4 = 127.5 -> 127 or 128
    // B = (0+0+255+255)/4 = 127.5 -> 127 or 128
    // A = (255+255+255+255)/4 = 255
    // Actual core lib ShrinkPixel averages then assigns. (255+0+0+255)/4 = 510/4 = 127
    auto p00 = getPixel(shrunk_bmp, 0, 0);
    EXPECT_EQ(p00[0], (255+0+0+255)/4); // R
    EXPECT_EQ(p00[1], (0+255+0+255)/4); // G
    EXPECT_EQ(p00[2], (0+0+255+255)/4); // B
    EXPECT_EQ(p00[3], (255+255+255+255)/4); // A
}

TEST(BmpToolApiManipulationTest, ShrinkScaleBy1) {
    BmpTool::Bitmap src;
    src.w = 2; src.h = 2; src.bpp = 32;
    src.data = {255,0,0,255, 0,255,0,255, 0,0,255,255, 10,20,30,40};

    auto result = BmpTool::shrink(src, 1);
    ASSERT_TRUE(result.isSuccess()) << "Shrink(1) failed: " << static_cast<int>(result.error());
    BmpTool::Bitmap shrunk_bmp = result.value();
    ASSERT_EQ(shrunk_bmp.w, src.w);
    ASSERT_EQ(shrunk_bmp.h, src.h);
    ASSERT_EQ(shrunk_bmp.bpp, src.bpp);
    ASSERT_EQ(shrunk_bmp.data, src.data); // Should be identical
}

TEST(BmpToolApiManipulationTest, ShrinkInvalidScaleFactor) {
    BmpTool::Bitmap src;
    src.w = 2; src.h = 2; src.bpp = 32; src.data.resize(2*2*4);
    auto result_zero = BmpTool::shrink(src, 0);
    ASSERT_TRUE(result_zero.isError());
    ASSERT_EQ(result_zero.error(), BmpTool::BitmapError::InvalidImageData);

    auto result_neg = BmpTool::shrink(src, -1);
    ASSERT_TRUE(result_neg.isError());
    ASSERT_EQ(result_neg.error(), BmpTool::BitmapError::InvalidImageData);
}

TEST(BmpToolApiManipulationTest, Greyscale) {
  BmpTool::Bitmap src;
  src.w = 1; src.h = 1; src.bpp = 32;
  src.data = {255, 128, 64, 250}; // R,G,B,A
  auto result = BmpTool::greyscale(src);
  ASSERT_TRUE(result.isSuccess()) << "Greyscale failed: " << static_cast<int>(result.error());
  BmpTool::Bitmap grey_bmp = result.value();

  ASSERT_EQ(grey_bmp.w, 1);
  ASSERT_EQ(grey_bmp.h, 1);
  ASSERT_EQ(grey_bmp.bpp, 32);
  ASSERT_EQ(grey_bmp.data.size(), 4);

  // Core lib ::GreyscalePixel uses: avg = (R + G + B) / 3;
  uint8_t expected_grey = static_cast<uint8_t>(std::round((255.0 + 128.0 + 64.0) / 3.0));
  // (255+128+64)/3 = 447/3 = 149
  auto p00 = getPixel(grey_bmp, 0, 0);
  EXPECT_EQ(p00[0], 149); // R
  EXPECT_EQ(p00[1], 149); // G
  EXPECT_EQ(p00[2], 149); // B
  EXPECT_EQ(p00[3], 250); // Alpha should be preserved
}

TEST(BmpToolApiManipulationTest, InvertColors) {
  BmpTool::Bitmap src;
  src.w = 1; src.h = 1; src.bpp = 32;
  src.data = {200, 100, 50, 150}; // R,G,B,A
  auto result = BmpTool::invertColors(src);
  ASSERT_TRUE(result.isSuccess()) << "InvertColors failed: " << static_cast<int>(result.error());
  BmpTool::Bitmap inverted_bmp = result.value();

  ASSERT_EQ(inverted_bmp.w, 1);
  ASSERT_EQ(inverted_bmp.h, 1);
  ASSERT_EQ(inverted_bmp.bpp, 32);
  ASSERT_EQ(inverted_bmp.data.size(), 4);

  auto p00 = getPixel(inverted_bmp, 0, 0);
  EXPECT_EQ(p00[0], 255 - 200); // R
  EXPECT_EQ(p00[1], 255 - 100); // G
  EXPECT_EQ(p00[2], 255 - 50);  // B
  EXPECT_EQ(p00[3], 150);       // Alpha unchanged
}

TEST(BmpToolApiManipulationTest, RotateClockwiseNonSquare) {
    BmpTool::Bitmap src; // 2x1 bitmap
    src.w = 2; src.h = 1; src.bpp = 32;
    // P(0,0)=Red, P(1,0)=Green
    src.data = {255,0,0,255,  0,255,0,255};

    auto result = BmpTool::rotateClockwise(src);
    ASSERT_TRUE(result.isSuccess()) << "RotateClockwise failed: " << static_cast<int>(result.error());
    BmpTool::Bitmap rotated_bmp = result.value();

    ASSERT_EQ(rotated_bmp.w, 1); // Old height
    ASSERT_EQ(rotated_bmp.h, 2); // Old width
    ASSERT_EQ(rotated_bmp.bpp, 32);
    ASSERT_EQ(rotated_bmp.data.size(), 1 * 2 * 4);

    // Original P(0,0) Red moves to New P(0,0)
    // Original P(1,0) Green moves to New P(0,1)
    auto p00_new = getPixel(rotated_bmp, 0, 0); // Should be Red
    EXPECT_EQ(p00_new[0], 255); EXPECT_EQ(p00_new[1], 0); EXPECT_EQ(p00_new[2], 0); EXPECT_EQ(p00_new[3], 255);

    auto p01_new = getPixel(rotated_bmp, 0, 1); // Should be Green
    EXPECT_EQ(p01_new[0], 0); EXPECT_EQ(p01_new[1], 255); EXPECT_EQ(p01_new[2], 0); EXPECT_EQ(p01_new[3], 255);
}


TEST(BmpToolApiManipulationTest, ApplyBoxBlurRadius0) {
    BmpTool::Bitmap src;
    src.w = 2; src.h = 2; src.bpp = 32;
    src.data = {255,0,0,255, 0,255,0,255, 0,0,255,255, 10,20,30,40};

    auto result = BmpTool::applyBoxBlur(src, 0);
    ASSERT_TRUE(result.isSuccess()) << "applyBoxBlur(0) failed: " << static_cast<int>(result.error());
    BmpTool::Bitmap blurred_bmp = result.value();
    ASSERT_EQ(blurred_bmp.w, src.w);
    ASSERT_EQ(blurred_bmp.h, src.h);
    ASSERT_EQ(blurred_bmp.data, src.data); // Should be identical
}

TEST(BmpToolApiManipulationTest, ApplyBoxBlurRadius1) {
    BmpTool::Bitmap src; // 3x1 bitmap
    src.w = 3; src.h = 1; src.bpp = 32;
    // P0=Red(255,0,0), P1=Green(0,255,0), P2=Blue(0,0,255) all alpha 255
    src.data = {
        255,0,0,255,   0,255,0,255,   0,0,255,255
    };

    auto result = BmpTool::applyBoxBlur(src, 1);
    ASSERT_TRUE(result.isSuccess()) << "applyBoxBlur(1) failed: " << static_cast<int>(result.error());
    BmpTool::Bitmap blurred_bmp = result.value();

    ASSERT_EQ(blurred_bmp.w, src.w);
    ASSERT_EQ(blurred_bmp.h, src.h);
    ASSERT_EQ(blurred_bmp.data.size(), src.data.size());

    // Central pixel P1(1,0) blurred. Neighborhood is P0, P1, P2 (since it's 1D effectively for radius 1 on a 1-row image)
    // R_avg = (255+0+0)/3 = 85
    // G_avg = (0+255+0)/3 = 85
    // B_avg = (0+0+255)/3 = 85
    // Alpha_avg = (255+255+255)/3 = 255
    auto p1_blurred = getPixel(blurred_bmp, 1, 0);
    EXPECT_EQ(p1_blurred[0], 85);
    EXPECT_EQ(p1_blurred[1], 85);
    EXPECT_EQ(p1_blurred[2], 85);
    EXPECT_EQ(p1_blurred[3], 255);

    // Edge pixel P0(0,0) blurred. Neighborhood is P0, P1 (kernel clamps at edges)
    // R_avg = (255+0)/2 = 127 (integer division)
    // G_avg = (0+255)/2 = 127
    // B_avg = (0+0)/2 = 0
    // Alpha_avg = (255+255)/2 = 255
    auto p0_blurred = getPixel(blurred_bmp, 0, 0);
    EXPECT_EQ(p0_blurred[0], 127);
    EXPECT_EQ(p0_blurred[1], 127);
    EXPECT_EQ(p0_blurred[2], 0);
    EXPECT_EQ(p0_blurred[3], 255);
}

TEST(BmpToolApiManipulationTest, ApplyBoxBlurInvalidRadius) {
    BmpTool::Bitmap src;
    src.w = 2; src.h = 2; src.bpp = 32; src.data.resize(2*2*4);
    auto result = BmpTool::applyBoxBlur(src, -1);
    ASSERT_TRUE(result.isError());
    ASSERT_EQ(result.error(), BmpTool::BitmapError::InvalidImageData);
}

// Test for an empty bitmap input
TEST(BmpToolApiManipulationTest, EmptyBitmapInput) {
    BmpTool::Bitmap src; // Default constructed: w=0, h=0, bpp=0, empty data

    auto result_shrink = BmpTool::shrink(src, 2);
    ASSERT_TRUE(result_shrink.isError());
    ASSERT_EQ(result_shrink.error(), BmpTool::BitmapError::InvalidImageData);

    auto result_greyscale = BmpTool::greyscale(src);
    ASSERT_TRUE(result_greyscale.isError());
    ASSERT_EQ(result_greyscale.error(), BmpTool::BitmapError::InvalidImageData);

    // Add one more for good measure
    auto result_invert = BmpTool::invertColors(src);
    ASSERT_TRUE(result_invert.isError());
    ASSERT_EQ(result_invert.error(), BmpTool::BitmapError::InvalidImageData);
}

TEST(BmpToolApiManipulationTest, BitmapWithZeroDimension) {
    BmpTool::Bitmap src;
    src.w = 0; src.h = 10; src.bpp = 32; src.data.clear(); // No data for zero width

    auto result_rotate = BmpTool::rotateClockwise(src);
    ASSERT_TRUE(result_rotate.isError());
    ASSERT_EQ(result_rotate.error(), BmpTool::BitmapError::InvalidImageData);

    src.w = 10; src.h = 0; src.bpp = 32; src.data.clear();
    auto result_flip = BmpTool::flip(src);
    ASSERT_TRUE(result_flip.isError());
    ASSERT_EQ(result_flip.error(), BmpTool::BitmapError::InvalidImageData);
}

TEST(BmpToolApiManipulationTest, BitmapWithWrongBpp) {
    BmpTool::Bitmap src;
    src.w = 1; src.h = 1; src.bpp = 24; // Wrong bpp for this API
    src.data.resize(1*1*3); // 3 bytes for 24bpp

    auto result_mirror = BmpTool::mirror(src);
    ASSERT_TRUE(result_mirror.isError());
    ASSERT_EQ(result_mirror.error(), BmpTool::BitmapError::InvalidImageData);
}

TEST(BmpToolApiManipulationTest, BitmapWithInsufficientData) {
    BmpTool::Bitmap src;
    src.w = 2; src.h = 2; src.bpp = 32;
    src.data.resize(2*2*4 - 1); // One byte less than required

    auto result_sepia = BmpTool::applySepiaTone(src);
    ASSERT_TRUE(result_sepia.isError());
    ASSERT_EQ(result_sepia.error(), BmpTool::BitmapError::InvalidImageData);
}

// TODO: Add more specific tests for color manipulation functions
// (changeBrightness, changeContrast, changeSaturation variants, changeLuminance variants)
// For these, checking a single pixel's transformation against expected values would be good.
// Example for changeBrightness:
TEST(BmpToolApiManipulationTest, ChangeBrightness) {
    BmpTool::Bitmap src;
    src.w = 1; src.h = 1; src.bpp = 32;
    src.data = {100, 150, 200, 255}; // R,G,B,A

    auto result = BmpTool::changeBrightness(src, 1.2f); // Increase brightness by 20%
    ASSERT_TRUE(result.isSuccess()) << "ChangeBrightness failed: " << static_cast<int>(result.error());
    BmpTool::Bitmap bright_bmp = result.value();

    auto p00 = getPixel(bright_bmp, 0, 0);
    // Expected: R = 100*1.2=120, G = 150*1.2=180, B = 200*1.2=240. Alpha unchanged.
    // Clamping at 255 should be handled by core lib.
    EXPECT_EQ(p00[0], static_cast<uint8_t>(std::min(255.0f, 100.0f * 1.2f)));
    EXPECT_EQ(p00[1], static_cast<uint8_t>(std::min(255.0f, 150.0f * 1.2f)));
    EXPECT_EQ(p00[2], static_cast<uint8_t>(std::min(255.0f, 200.0f * 1.2f)));
    EXPECT_EQ(p00[3], 255);
}

TEST(BmpToolApiManipulationTest, ChangeBrightnessDarkenAndClamp) {
    BmpTool::Bitmap src;
    src.w = 1; src.h = 1; src.bpp = 32;
    src.data = {10, 20, 300 /*invalid, but test clamping in core*/, 255};
    src.data[2] = 250; // Valid original blue

    auto result = BmpTool::changeBrightness(src, 0.5f); // Decrease brightness by 50%
    ASSERT_TRUE(result.isSuccess()) << "ChangeBrightness failed: " << static_cast<int>(result.error());
    BmpTool::Bitmap dark_bmp = result.value();

    auto p00 = getPixel(dark_bmp, 0, 0);
    EXPECT_EQ(p00[0], static_cast<uint8_t>(10.0f * 0.5f)); // 5
    EXPECT_EQ(p00[1], static_cast<uint8_t>(20.0f * 0.5f)); // 10
    EXPECT_EQ(p00[2], static_cast<uint8_t>(250.0f * 0.5f)); // 125
    EXPECT_EQ(p00[3], 255);

    // Test clamping at 0 (though factor is positive, if original was 0)
    src.data = {0,0,0,255};
    result = BmpTool::changeBrightness(src, 0.1f);
    ASSERT_TRUE(result.isSuccess());
    dark_bmp = result.value();
    p00 = getPixel(dark_bmp, 0,0);
    EXPECT_EQ(p00[0], 0);
    EXPECT_EQ(p00[1], 0);
    EXPECT_EQ(p00[2], 0);
}
