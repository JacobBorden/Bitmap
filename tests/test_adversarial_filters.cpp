#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include <cmath>
#include <set>
#include <algorithm>
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

// ---------------------------------------------------------------------------
// Week 2 Day 7: Non-Linear Median Filter Tests
// ---------------------------------------------------------------------------

namespace {

// Reference 9-element sorting network helper matching computeMedian9
inline void sort2_ref(uint8_t& a, uint8_t& b) {
    if (a > b) std::swap(a, b);
}

inline uint8_t networkMedian9(uint8_t a0, uint8_t a1, uint8_t a2,
                              uint8_t a3, uint8_t a4, uint8_t a5,
                              uint8_t a6, uint8_t a7, uint8_t a8) {
    uint8_t a[9] = {a0, a1, a2, a3, a4, a5, a6, a7, a8};
    sort2_ref(a[0], a[1]); sort2_ref(a[2], a[3]);
    sort2_ref(a[0], a[2]); sort2_ref(a[1], a[3]);
    sort2_ref(a[1], a[2]);

    sort2_ref(a[4], a[5]); sort2_ref(a[6], a[7]);
    sort2_ref(a[4], a[6]); sort2_ref(a[5], a[7]);
    sort2_ref(a[5], a[6]);

    sort2_ref(a[0], a[4]); sort2_ref(a[1], a[5]); sort2_ref(a[2], a[6]); sort2_ref(a[3], a[7]);
    sort2_ref(a[2], a[4]); sort2_ref(a[3], a[5]);
    sort2_ref(a[1], a[2]); sort2_ref(a[3], a[4]); sort2_ref(a[5], a[6]);

    return std::clamp(a[8], a[3], a[4]);
}

} // namespace

TEST(AdversarialFilterTest, MedianFilter3x3_ZeroOnePrinciple) {
    // 0-1 Sorting Lemma: If a comparator network correctly sorts / selects on all 2^9 = 512
    // binary sequences of 0s and 1s, it is mathematically proven to work for all inputs.
    for (int mask = 0; mask < (1 << 9); ++mask) {
        uint8_t bits[9];
        for (int i = 0; i < 9; ++i) {
            bits[i] = static_cast<uint8_t>((mask >> i) & 1);
        }

        uint8_t expected_w[9];
        std::copy(bits, bits + 9, expected_w);
        std::nth_element(expected_w, expected_w + 4, expected_w + 9);
        uint8_t expected_median = expected_w[4];

        uint8_t actual_median = networkMedian9(bits[0], bits[1], bits[2],
                                               bits[3], bits[4], bits[5],
                                               bits[6], bits[7], bits[8]);

        ASSERT_EQ(actual_median, expected_median)
            << "Network median mismatch at binary mask: " << mask;
    }
}

TEST(AdversarialFilterTest, MedianFilter3x3_SinglePixelImpulseRemoval) {
    // In a 5x5 image of uniform gray (100, 100, 100), corrupt the center (2, 2)
    // with extreme white impulse noise (255, 255, 255).
    auto bmp = createTestBitmap32(5, 5, 100, 100, 100, 255);
    const size_t center_offset = (2 * 5 + 2) * 4;
    bmp.data[center_offset]     = 255;
    bmp.data[center_offset + 1] = 255;
    bmp.data[center_offset + 2] = 255;

    auto res = BmpTool::medianFilter(bmp, 3);
    ASSERT_TRUE(res.isSuccess());
    const auto& filtered = res.value();

    // Center pixel must be completely restored to 100
    EXPECT_EQ(filtered.data[center_offset], 100);
    EXPECT_EQ(filtered.data[center_offset + 1], 100);
    EXPECT_EQ(filtered.data[center_offset + 2], 100);
    EXPECT_EQ(filtered.data[center_offset + 3], 255);

    // The surrounding pixels must remain clean 100
    for (size_t i = 0; i < filtered.data.size(); i += 4) {
        EXPECT_EQ(filtered.data[i], 100);
        EXPECT_EQ(filtered.data[i + 1], 100);
        EXPECT_EQ(filtered.data[i + 2], 100);
    }
}

TEST(AdversarialFilterTest, MedianFilter5x5_ImpulseRemoval) {
    // 5x5 kernel has 25 elements; can eliminate up to 12 corrupted neighbor pixels.
    // In a 7x7 image of uniform color (40, 60, 80), corrupt a 2x2 cluster with salt noise (255)
    auto bmp = createTestBitmap32(7, 7, 40, 60, 80, 255);
    bmp.data[(3 * 7 + 3) * 4] = 255;
    bmp.data[(3 * 7 + 4) * 4] = 255;
    bmp.data[(4 * 7 + 3) * 4] = 255;
    bmp.data[(4 * 7 + 4) * 4] = 255;

    auto res = BmpTool::medianFilter(bmp, 5);
    ASSERT_TRUE(res.isSuccess());
    const auto& filtered = res.value();

    // In 5x5 window (25 pixels), 4 salt pixels cannot overpower 21 clean pixels
    EXPECT_EQ(filtered.data[(3 * 7 + 3) * 4], 40);
    EXPECT_EQ(filtered.data[(3 * 7 + 4) * 4], 40);
    EXPECT_EQ(filtered.data[(4 * 7 + 3) * 4], 40);
    EXPECT_EQ(filtered.data[(4 * 7 + 4) * 4], 40);
}

TEST(AdversarialFilterTest, MedianFilter_EdgePreservationVsBoxBlur) {
    // Construct a 6x6 image with a sharp vertical edge:
    // Left half (cols 0, 1, 2) = 50, Right half (cols 3, 4, 5) = 200.
    BmpTool::Bitmap bmp;
    bmp.w = 6;
    bmp.h = 6;
    bmp.bpp = 32;
    bmp.data.resize(6 * 6 * 4);

    for (uint32_t y = 0; y < 6; ++y) {
        for (uint32_t x = 0; x < 6; ++x) {
            uint8_t val = (x < 3) ? 50 : 200;
            size_t idx = (y * 6 + x) * 4;
            bmp.data[idx]     = val;
            bmp.data[idx + 1] = val;
            bmp.data[idx + 2] = val;
            bmp.data[idx + 3] = 255;
        }
    }

    auto median_res = BmpTool::medianFilter(bmp, 3);
    ASSERT_TRUE(median_res.isSuccess());
    const auto& med = median_res.value();

    // Under median filtering, the step edge remains completely sharp!
    // Row 2, Col 2 must be 50; Row 2, Col 3 must be 200.
    EXPECT_EQ(med.data[(2 * 6 + 2) * 4], 50);
    EXPECT_EQ(med.data[(2 * 6 + 3) * 4], 200);

    // Box blur would blur the boundary pixels (e.g. (50+50+50+50+50+50+200+200+200)/9 = 100)
    auto blur_res = BmpTool::applyBoxBlur(bmp, 1);
    ASSERT_TRUE(blur_res.isSuccess());
    const auto& blurred = blur_res.value();
    EXPECT_NE(blurred.data[(2 * 6 + 2) * 4], 50);
    EXPECT_NE(blurred.data[(2 * 6 + 3) * 4], 200);
}

TEST(AdversarialFilterTest, MedianFilter_AlphaPreservation) {
    auto bmp = createTestBitmap32(3, 3, 100, 100, 100, 77);
    // Center pixel with different alpha and noise
    bmp.data[(1 * 3 + 1) * 4]     = 250; // impulse noise
    bmp.data[(1 * 3 + 1) * 4 + 3] = 77;

    // preserveAlpha = true (default)
    auto res = BmpTool::medianFilter(bmp, 3, true);
    ASSERT_TRUE(res.isSuccess());
    const auto& filtered = res.value();
    EXPECT_EQ(filtered.data[(1 * 3 + 1) * 4], 100);     // Color denoised
    EXPECT_EQ(filtered.data[(1 * 3 + 1) * 4 + 3], 77);  // Alpha preserved exactly
}

TEST(AdversarialFilterTest, MedianFilter_AlphaFiltering) {
    auto bmp = createTestBitmap32(3, 3, 100, 100, 100, 100);
    // Center pixel has corrupt alpha
    bmp.data[(1 * 3 + 1) * 4 + 3] = 0; // alpha spike

    // preserveAlpha = false
    auto res = BmpTool::medianFilter(bmp, 3, false);
    ASSERT_TRUE(res.isSuccess());
    const auto& filtered = res.value();
    // Corrupt alpha 0 in a sea of eight 100s is filtered out to 100!
    EXPECT_EQ(filtered.data[(1 * 3 + 1) * 4 + 3], 100);
}

TEST(AdversarialFilterTest, MedianFilter_24bpp) {
    auto bmp = createTestBitmap24(5, 5, 30, 60, 90);
    // Add salt noise at (2, 2)
    bmp.data[(2 * 5 + 2) * 3]     = 255;
    bmp.data[(2 * 5 + 2) * 3 + 1] = 255;
    bmp.data[(2 * 5 + 2) * 3 + 2] = 255;

    auto res = BmpTool::medianFilter(bmp, 3);
    ASSERT_TRUE(res.isSuccess());
    const auto& filtered = res.value();
    EXPECT_EQ(filtered.bpp, 24u);
    EXPECT_EQ(filtered.data[(2 * 5 + 2) * 3], 30);
    EXPECT_EQ(filtered.data[(2 * 5 + 2) * 3 + 1], 60);
    EXPECT_EQ(filtered.data[(2 * 5 + 2) * 3 + 2], 90);
}

TEST(AdversarialFilterTest, MedianFilter_BoundaryAndErrorRejection) {
    auto bmp = createTestBitmap32(3, 3, 100, 100, 100);

    // Invalid kernel sizes (only 3 and 5 are valid)
    EXPECT_TRUE(BmpTool::medianFilter(bmp, 0).isError());
    EXPECT_EQ(BmpTool::medianFilter(bmp, 0).error(), BmpTool::BitmapError::InvalidImageData);

    EXPECT_TRUE(BmpTool::medianFilter(bmp, 1).isError());
    EXPECT_EQ(BmpTool::medianFilter(bmp, 1).error(), BmpTool::BitmapError::InvalidImageData);

    EXPECT_TRUE(BmpTool::medianFilter(bmp, 2).isError());
    EXPECT_EQ(BmpTool::medianFilter(bmp, 2).error(), BmpTool::BitmapError::InvalidImageData);

    EXPECT_TRUE(BmpTool::medianFilter(bmp, 4).isError());
    EXPECT_EQ(BmpTool::medianFilter(bmp, 4).error(), BmpTool::BitmapError::InvalidImageData);

    EXPECT_TRUE(BmpTool::medianFilter(bmp, 7).isError());
    EXPECT_EQ(BmpTool::medianFilter(bmp, 7).error(), BmpTool::BitmapError::InvalidImageData);

    // Zero dimensions
    auto zero_w = bmp;
    zero_w.w = 0;
    EXPECT_TRUE(BmpTool::medianFilter(zero_w, 3).isError());
    EXPECT_EQ(BmpTool::medianFilter(zero_w, 3).error(), BmpTool::BitmapError::InvalidImageData);

    auto zero_h = bmp;
    zero_h.h = 0;
    EXPECT_TRUE(BmpTool::medianFilter(zero_h, 3).isError());
    EXPECT_EQ(BmpTool::medianFilter(zero_h, 3).error(), BmpTool::BitmapError::InvalidImageData);

    // Unsupported BPP
    auto bad_bpp = bmp;
    bad_bpp.bpp = 16;
    EXPECT_TRUE(BmpTool::medianFilter(bad_bpp, 3).isError());
    EXPECT_EQ(BmpTool::medianFilter(bad_bpp, 3).error(), BmpTool::BitmapError::InvalidColorDepth);

    // Exceeds MAX_SAFE_DIMENSION
    auto huge = bmp;
    huge.w = BmpTool::SafeMath::MAX_SAFE_DIMENSION + 1;
    EXPECT_TRUE(BmpTool::medianFilter(huge, 3).isError());
    EXPECT_EQ(BmpTool::medianFilter(huge, 3).error(), BmpTool::BitmapError::ExceedsMaxDimensions);

    // Truncated buffer
    auto trunc = bmp;
    trunc.data.pop_back();
    EXPECT_TRUE(BmpTool::medianFilter(trunc, 3).isError());
    EXPECT_EQ(BmpTool::medianFilter(trunc, 3).error(), BmpTool::BitmapError::InvalidImageData);
}

