#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include <cmath>
#include <set>
#include "../include/bitmap.hpp"
#include "../src/safe_math.hpp"

namespace {

// Helper to create a solid 32bpp Bitmap
BmpTool::Bitmap createTestBitmap32(uint32_t w, uint32_t h, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    BmpTool::Bitmap bmp;
    bmp.w = w;
    bmp.h = h;
    bmp.bpp = 32;
    bmp.data.resize(static_cast<size_t>(w) * h * 4);
    for (size_t i = 0; i < bmp.data.size(); i += 4) {
        bmp.data[i]     = r;
        bmp.data[i + 1] = g;
        bmp.data[i + 2] = b;
        bmp.data[i + 3] = a;
    }
    return bmp;
}

// Helper to create a solid 24bpp Bitmap
BmpTool::Bitmap createTestBitmap24(uint32_t w, uint32_t h, uint8_t r, uint8_t g, uint8_t b) {
    BmpTool::Bitmap bmp;
    bmp.w = w;
    bmp.h = h;
    bmp.bpp = 24;
    bmp.data.resize(static_cast<size_t>(w) * h * 3);
    for (size_t i = 0; i < bmp.data.size(); i += 3) {
        bmp.data[i]     = r;
        bmp.data[i + 1] = g;
        bmp.data[i + 2] = b;
    }
    return bmp;
}

} // namespace

TEST(AdversarialFilterTest, Quantize1BitBinarization) {
    // 1-bit per channel maps values < 128 to 0 and >= 128 to 255
    auto bmp = createTestBitmap32(2, 2, 50, 127, 128, 200);
    // Pixel 0: R=50 (<128 -> 0), G=127 (<128 -> 0), B=128 (>=128 -> 255), A=200
    // Modify pixel (1, 1) to test extreme values
    bmp.data[12] = 0;
    bmp.data[13] = 254;
    bmp.data[14] = 255;
    bmp.data[15] = 100;

    auto res = BmpTool::quantizeChannels(bmp, 1, false);
    ASSERT_TRUE(res.isSuccess());
    const auto& q = res.value();

    // Pixel (0, 0): R=0, G=0, B=255, Alpha preserved at 200
    EXPECT_EQ(q.data[0], 0);
    EXPECT_EQ(q.data[1], 0);
    EXPECT_EQ(q.data[2], 255);
    EXPECT_EQ(q.data[3], 200);

    // Pixel (1, 1): R=0, G=255, B=255, Alpha preserved at 100
    EXPECT_EQ(q.data[12], 0);
    EXPECT_EQ(q.data[13], 255);
    EXPECT_EQ(q.data[14], 255);
    EXPECT_EQ(q.data[15], 100);
}

TEST(AdversarialFilterTest, Quantize4BitCoarseBuckets) {
    // 4-bit per channel produces exactly 16 levels: 0, 17, 34, ..., 255 (multiples of 17)
    BmpTool::Bitmap bmp;
    bmp.w = 256;
    bmp.h = 1;
    bmp.bpp = 32;
    bmp.data.resize(256 * 4);

    // Fill image with a ramp of all 256 byte values
    for (uint32_t i = 0; i < 256; ++i) {
        bmp.data[i * 4]     = static_cast<uint8_t>(i); // R
        bmp.data[i * 4 + 1] = static_cast<uint8_t>(i); // G
        bmp.data[i * 4 + 2] = static_cast<uint8_t>(i); // B
        bmp.data[i * 4 + 3] = 255;                     // A
    }

    auto res = BmpTool::quantizeChannels(bmp, 4, false);
    ASSERT_TRUE(res.isSuccess());
    const auto& q = res.value();

    std::set<uint8_t> distinct_levels;
    for (uint32_t i = 0; i < 256; ++i) {
        uint8_t r = q.data[i * 4];
        distinct_levels.insert(r);
        // Every level must be an exact multiple of 17
        EXPECT_EQ(r % 17, 0) << "Value " << static_cast<int>(r) << " is not a multiple of 17";
    }

    // Exactly 16 distinct levels
    EXPECT_EQ(distinct_levels.size(), 16u);
    EXPECT_EQ(*distinct_levels.begin(), 0);
    EXPECT_EQ(*distinct_levels.rbegin(), 255);
}

TEST(AdversarialFilterTest, Quantize8BitIdentity) {
    // 8-bit per channel must be an exact identity operation
    auto bmp = createTestBitmap32(4, 4, 11, 42, 99, 173);
    auto res = BmpTool::quantizeChannels(bmp, 8, false);
    ASSERT_TRUE(res.isSuccess());
    EXPECT_EQ(res.value().data, bmp.data);
}

TEST(AdversarialFilterTest, AlphaChannelPreservation) {
    // Verify Alpha is untouched when quantizeAlpha is false
    auto bmp = createTestBitmap32(2, 2, 200, 200, 200, 77);
    auto res = BmpTool::quantizeChannels(bmp, 2, false); // 2-bit color
    ASSERT_TRUE(res.isSuccess());
    EXPECT_EQ(res.value().data[3], 77); // Alpha preserved
}

TEST(AdversarialFilterTest, AlphaChannelQuantization) {
    // Verify Alpha is quantized when quantizeAlpha is true
    auto bmp = createTestBitmap32(2, 2, 200, 200, 200, 77);
    auto res = BmpTool::quantizeChannels(bmp, 2, true); // 2-bit color and alpha
    ASSERT_TRUE(res.isSuccess());
    // 2-bit levels: 0, 85, 170, 255. 77 / 85 = 0.905 -> 85
    EXPECT_EQ(res.value().data[3], 85);
}

TEST(AdversarialFilterTest, AdversarialNoiseSuppression) {
    // Simulate an adversarial gradient perturbation (e.g., FGSM with epsilon = +-3)
    // Clean image
    auto clean_bmp = createTestBitmap32(2, 2, 100, 150, 200);

    // Attacked image with micro-perturbations added to fool a classifier
    auto attacked_bmp = clean_bmp;
    attacked_bmp.data[0] += 3; // 103 instead of 100
    attacked_bmp.data[1] -= 3; // 147 instead of 150
    attacked_bmp.data[2] += 2; // 202 instead of 200

    // Feature squeezing with 4 bits per channel (16 levels, step size 17)
    auto clean_quantized = BmpTool::quantizeChannels(clean_bmp, 4);
    auto attacked_quantized = BmpTool::quantizeChannels(attacked_bmp, 4);

    ASSERT_TRUE(clean_quantized.isSuccess());
    ASSERT_TRUE(attacked_quantized.isSuccess());

    // Both clean and attacked pixels should collapse to the EXACT same bucket:
    // 100 / 17 = 5.88 -> 6 * 17 = 102
    // 103 / 17 = 6.05 -> 6 * 17 = 102
    // 150 / 17 = 8.82 -> 9 * 17 = 153
    // 147 / 17 = 8.64 -> 9 * 17 = 153
    // 200 / 17 = 11.76 -> 12 * 17 = 204
    // 202 / 17 = 11.88 -> 12 * 17 = 204
    EXPECT_EQ(clean_quantized.value().data[0], 102);
    EXPECT_EQ(attacked_quantized.value().data[0], 102);

    EXPECT_EQ(clean_quantized.value().data[1], 153);
    EXPECT_EQ(attacked_quantized.value().data[1], 153);

    EXPECT_EQ(clean_quantized.value().data[2], 204);
    EXPECT_EQ(attacked_quantized.value().data[2], 204);

    // The entire pixel buffer is now identical, neutralizing the adversarial perturbation!
    EXPECT_EQ(clean_quantized.value().data, attacked_quantized.value().data);
}

TEST(AdversarialFilterTest, Quantize24bppBitmap) {
    auto bmp = createTestBitmap24(3, 3, 100, 150, 200);
    auto res = BmpTool::quantizeChannels(bmp, 4);
    ASSERT_TRUE(res.isSuccess());
    const auto& q = res.value();
    EXPECT_EQ(q.bpp, 24u);
    EXPECT_EQ(q.w, 3u);
    EXPECT_EQ(q.h, 3u);
    EXPECT_EQ(q.data.size(), 3 * 3 * 3u);
    EXPECT_EQ(q.data[0], 102);
    EXPECT_EQ(q.data[1], 153);
    EXPECT_EQ(q.data[2], 204);
}

TEST(AdversarialFilterTest, BoundaryAndErrorRejection) {
    auto bmp = createTestBitmap32(2, 2, 100, 100, 100);

    // Invalid bit depth: 0
    auto res = BmpTool::quantizeChannels(bmp, 0);
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidColorDepth);

    // Invalid bit depth: 9
    res = BmpTool::quantizeChannels(bmp, 9);
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidColorDepth);

    // Invalid bit depth: 255
    res = BmpTool::quantizeChannels(bmp, 255);
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidColorDepth);

    // Zero width
    auto zero_w = bmp;
    zero_w.w = 0;
    res = BmpTool::quantizeChannels(zero_w, 4);
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidImageData);

    // Zero height
    auto zero_h = bmp;
    zero_h.h = 0;
    res = BmpTool::quantizeChannels(zero_h, 4);
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidImageData);

    // Unsupported bpp
    auto bad_bpp = bmp;
    bad_bpp.bpp = 16;
    res = BmpTool::quantizeChannels(bad_bpp, 4);
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidColorDepth);

    // Exceeds max dimensions
    auto huge_bmp = bmp;
    huge_bmp.w = BmpTool::SafeMath::MAX_SAFE_DIMENSION + 1;
    res = BmpTool::quantizeChannels(huge_bmp, 4);
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error(), BmpTool::BitmapError::ExceedsMaxDimensions);

    // Truncated buffer
    auto trunc_bmp = bmp;
    trunc_bmp.data.pop_back();
    res = BmpTool::quantizeChannels(trunc_bmp, 4);
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidImageData);
}
