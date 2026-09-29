#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include <cmath>
#include <algorithm>
#include "../include/bitmap.hpp"
#include "../src/bitmap/bitmap.h"
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

// ===========================================================================
// Day 11: Bilinear Interpolation Resampling Tests
// ===========================================================================

TEST(VisionPreprocessingTest, BilinearResize_Identity) {
    auto bmp = createTestBitmap32(8, 8, 42, 84, 126, 255);
    auto res = BmpTool::resizeBilinear(bmp, 8, 8);
    ASSERT_TRUE(res.isSuccess());
    EXPECT_EQ(res.value().w, 8u);
    EXPECT_EQ(res.value().h, 8u);
    EXPECT_EQ(res.value().data, bmp.data);
}

TEST(VisionPreprocessingTest, BilinearResize_UniformColor) {
    // Both downscaling and upscaling of solid color must strictly conserve pixel values
    auto bmp32 = createTestBitmap32(16, 16, 100, 150, 200, 255);
    auto down32 = BmpTool::resizeBilinear(bmp32, 4, 4);
    ASSERT_TRUE(down32.isSuccess());
    for (size_t i = 0; i < down32.value().data.size(); i += 4) {
        EXPECT_EQ(down32.value().data[i], 100);
        EXPECT_EQ(down32.value().data[i + 1], 150);
        EXPECT_EQ(down32.value().data[i + 2], 200);
        EXPECT_EQ(down32.value().data[i + 3], 255);
    }

    auto up32 = BmpTool::resizeBilinear(bmp32, 32, 32);
    ASSERT_TRUE(up32.isSuccess());
    for (size_t i = 0; i < up32.value().data.size(); i += 4) {
        EXPECT_EQ(up32.value().data[i], 100);
        EXPECT_EQ(up32.value().data[i + 1], 150);
        EXPECT_EQ(up32.value().data[i + 2], 200);
        EXPECT_EQ(up32.value().data[i + 3], 255);
    }
}

TEST(VisionPreprocessingTest, BilinearResize_ExactMathematicalInterpolation) {
    // 2x2 image with distinct values at 4 corners:
    // (0,0) = 0,    (1,0) = 100
    // (0,1) = 200,  (1,1) = 40
    BmpTool::Bitmap bmp;
    bmp.w = 2;
    bmp.h = 2;
    bmp.bpp = 32;
    bmp.data.resize(2 * 2 * 4);

    auto set_px = [&](uint32_t x, uint32_t y, uint8_t val) {
        size_t idx = (y * 2 + x) * 4;
        bmp.data[idx]     = val;
        bmp.data[idx + 1] = val;
        bmp.data[idx + 2] = val;
        bmp.data[idx + 3] = 255;
    };
    set_px(0, 0, 0);
    set_px(1, 0, 100);
    set_px(0, 1, 200);
    set_px(1, 1, 40);

    // Upscale to 4x4
    auto res = BmpTool::resizeBilinear(bmp, 4, 4);
    ASSERT_TRUE(res.isSuccess());
    const auto& dst = res.value();
    EXPECT_EQ(dst.w, 4u);
    EXPECT_EQ(dst.h, 4u);

    // Center 2x2 pixels of the 4x4 destination correspond to:
    // X=1.5, Y=1.5 in destination coordinates.
    // In continuous sub-pixel source space:
    // For X=1: src_x = (1 + 0.5) * (2/4) - 0.5 = 0.25 (dx = 0.25)
    // For X=2: src_x = (2 + 0.5) * (2/4) - 0.5 = 0.75 (dx = 0.75)
    // For Y=1: src_y = 0.25 (dy = 0.25)
    // For Y=2: src_y = 0.75 (dy = 0.75)
    //
    // At (X=1, Y=1):
    // w00 = 0.75 * 0.75 = 0.5625
    // w10 = 0.25 * 0.75 = 0.1875
    // w01 = 0.75 * 0.25 = 0.1875
    // w11 = 0.25 * 0.25 = 0.0625
    // Expected val = 0.5625 * 0 + 0.1875 * 100 + 0.1875 * 200 + 0.0625 * 40
    //              = 0 + 18.75 + 37.5 + 2.5 = 58.75 -> 59
    size_t idx_1_1 = (1 * 4 + 1) * 4;
    EXPECT_NEAR(dst.data[idx_1_1], 59, 1);
}

TEST(VisionPreprocessingTest, BilinearResize_NeuralNetworkModelResolutions) {
    auto bmp = createTestBitmap32(100, 75, 50, 100, 150, 255);

    std::vector<std::pair<uint32_t, uint32_t>> resolutions = {
        {64, 64},     // Tiny CNN
        {128, 128},   // MobileNet V1 small
        {224, 224},   // ResNet50 / ViT / EfficientNet
        {640, 640}    // YOLOv8 / YOLOv9
    };

    for (const auto& [tw, th] : resolutions) {
        auto res = BmpTool::resizeBilinear(bmp, tw, th);
        ASSERT_TRUE(res.isSuccess());
        EXPECT_EQ(res.value().w, tw);
        EXPECT_EQ(res.value().h, th);
        EXPECT_EQ(res.value().bpp, 32u);
        EXPECT_EQ(res.value().data.size(), static_cast<size_t>(tw) * th * 4);
    }
}

TEST(VisionPreprocessingTest, BilinearResize_24bppAndAlphaPreservation) {
    auto bmp24 = createTestBitmap24(10, 10, 33, 66, 99);
    auto res24 = BmpTool::resizeBilinear(bmp24, 20, 20);
    ASSERT_TRUE(res24.isSuccess());
    EXPECT_EQ(res24.value().bpp, 24u);
    EXPECT_EQ(res24.value().data[0], 33);
    EXPECT_EQ(res24.value().data[1], 66);
    EXPECT_EQ(res24.value().data[2], 99);

    auto bmp32 = createTestBitmap32(10, 10, 33, 66, 99, 180);
    auto res32 = BmpTool::resizeBilinear(bmp32, 5, 5);
    ASSERT_TRUE(res32.isSuccess());
    EXPECT_EQ(res32.value().bpp, 32u);
    EXPECT_EQ(res32.value().data[3], 180); // Alpha preserved
}

TEST(VisionPreprocessingTest, BilinearResize_DegenerateAndEdgeClamping) {
    // Test 1x1 image upscaled to 5x5
    auto bmp1x1 = createTestBitmap32(1, 1, 77, 88, 99, 255);
    auto res1x1 = BmpTool::resizeBilinear(bmp1x1, 5, 5);
    ASSERT_TRUE(res1x1.isSuccess());
    for (size_t i = 0; i < res1x1.value().data.size(); i += 4) {
        EXPECT_EQ(res1x1.value().data[i], 77);
        EXPECT_EQ(res1x1.value().data[i + 1], 88);
        EXPECT_EQ(res1x1.value().data[i + 2], 99);
    }

    // 1x5 vertical stripe to 4x4
    auto bmp1x5 = createTestBitmap32(1, 5, 10, 20, 30);
    EXPECT_TRUE(BmpTool::resizeBilinear(bmp1x5, 4, 4).isSuccess());

    // 5x1 horizontal stripe to 4x4
    auto bmp5x1 = createTestBitmap32(5, 1, 10, 20, 30);
    EXPECT_TRUE(BmpTool::resizeBilinear(bmp5x1, 4, 4).isSuccess());
}

TEST(VisionPreprocessingTest, BilinearResize_ParameterValidation) {
    auto bmp = createTestBitmap32(4, 4, 10, 20, 30);

    // Target dimensions cannot be 0
    EXPECT_TRUE(BmpTool::resizeBilinear(bmp, 0, 4).isError());
    EXPECT_EQ(BmpTool::resizeBilinear(bmp, 0, 4).error(), BmpTool::BitmapError::InvalidImageData);

    EXPECT_TRUE(BmpTool::resizeBilinear(bmp, 4, 0).isError());
    EXPECT_EQ(BmpTool::resizeBilinear(bmp, 4, 0).error(), BmpTool::BitmapError::InvalidImageData);

    // Target dimensions exceeding MAX_SAFE_DIMENSION
    EXPECT_TRUE(BmpTool::resizeBilinear(bmp, BmpTool::SafeMath::MAX_SAFE_DIMENSION + 1, 4).isError());
    EXPECT_EQ(BmpTool::resizeBilinear(bmp, BmpTool::SafeMath::MAX_SAFE_DIMENSION + 1, 4).error(), BmpTool::BitmapError::ExceedsMaxDimensions);

    // Corrupted / truncated buffer
    auto trunc = bmp;
    trunc.data.pop_back();
    EXPECT_TRUE(BmpTool::resizeBilinear(trunc, 2, 2).isError());

    // Bad BPP
    auto bad_bpp = bmp;
    bad_bpp.bpp = 16;
    EXPECT_TRUE(BmpTool::resizeBilinear(bad_bpp, 2, 2).isError());
}

// ===========================================================================
// Day 12: Area-Averaging Resampling Tests
// ===========================================================================

TEST(VisionPreprocessingTest, AreaAveraging_UniformColorConservation) {
    // 32x32 uniform color downscaled to 4x4
    auto bmp = createTestBitmap32(32, 32, 114, 114, 114, 255);
    auto res = BmpTool::resizeAreaAveraging(bmp, 4, 4);
    ASSERT_TRUE(res.isSuccess());
    const auto& dst = res.value();
    EXPECT_EQ(dst.w, 4u);
    EXPECT_EQ(dst.h, 4u);
    for (size_t i = 0; i < dst.data.size(); i += 4) {
        EXPECT_EQ(dst.data[i], 114);
        EXPECT_EQ(dst.data[i + 1], 114);
        EXPECT_EQ(dst.data[i + 2], 114);
        EXPECT_EQ(dst.data[i + 3], 255);
    }
}

TEST(VisionPreprocessingTest, AreaAveraging_MoireAliasingSuppression) {
    // Create a high-frequency alternating checkerboard pattern (0 and 255)
    // 16x16 image where px(x, y) = ((x + y) % 2 == 0) ? 255 : 0
    const uint32_t w = 16, h = 16;
    BmpTool::Bitmap bmp;
    bmp.w = w;
    bmp.h = h;
    bmp.bpp = 32;
    bmp.data.resize(w * h * 4);

    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            uint8_t v = ((x + y) % 2 == 0) ? 255 : 0;
            size_t idx = (y * w + x) * 4;
            bmp.data[idx]     = v;
            bmp.data[idx + 1] = v;
            bmp.data[idx + 2] = v;
            bmp.data[idx + 3] = 255;
        }
    }

    // Downscale by non-integer factor (16x16 down to 4x4)
    // Since each 4x4 block contains an equal number of 0s and 255s,
    // area-averaging must converge smoothly to ~128 without false high-contrast Moiré stripes
    auto res = BmpTool::resizeAreaAveraging(bmp, 4, 4);
    ASSERT_TRUE(res.isSuccess());
    const auto& dst = res.value();

    for (size_t i = 0; i < dst.data.size(); i += 4) {
        EXPECT_NEAR(dst.data[i], 128, 2);
        EXPECT_NEAR(dst.data[i + 1], 128, 2);
        EXPECT_NEAR(dst.data[i + 2], 128, 2);
    }
}

TEST(VisionPreprocessingTest, AreaAveraging_EnergyConservation) {
    // Total integral of light energy should be preserved across downsampling:
    // Sum(pixel_val * pixel_area)
    const uint32_t src_w = 12, src_h = 12;
    const uint32_t dst_w = 4, dst_h = 4;
    auto bmp = createTestBitmap32(src_w, src_h, 0, 0, 0);

    // Fill with gradient: val = x * 15
    double src_total_energy = 0.0;
    for (uint32_t y = 0; y < src_h; ++y) {
        for (uint32_t x = 0; x < src_w; ++x) {
            uint8_t val = static_cast<uint8_t>(x * 15);
            size_t idx = (y * src_w + x) * 4;
            bmp.data[idx] = val;
            src_total_energy += val;
        }
    }
    double src_average = src_total_energy / (src_w * src_h);

    auto res = BmpTool::resizeAreaAveraging(bmp, dst_w, dst_h);
    ASSERT_TRUE(res.isSuccess());
    const auto& dst = res.value();

    double dst_total_energy = 0.0;
    for (size_t i = 0; i < dst.data.size(); i += 4) {
        dst_total_energy += dst.data[i];
    }
    double dst_average = dst_total_energy / (dst_w * dst_h);

    // Global mean must be conserved within 1 intensity count due to integer rounding
    EXPECT_NEAR(src_average, dst_average, 1.0);
}

TEST(VisionPreprocessingTest, AreaAveraging_SeamlessUpscalingFallback) {
    // When upscaling is requested, resizeAreaAveraging seamlessly blends via bilinear interpolation
    auto bmp = createTestBitmap32(2, 2, 50, 100, 150);
    auto res_up = BmpTool::resizeAreaAveraging(bmp, 8, 8);
    ASSERT_TRUE(res_up.isSuccess());
    EXPECT_EQ(res_up.value().w, 8u);
    EXPECT_EQ(res_up.value().h, 8u);
    // Values match bilinear output
    auto res_bilinear = BmpTool::resizeBilinear(bmp, 8, 8);
    ASSERT_TRUE(res_bilinear.isSuccess());
    EXPECT_EQ(res_up.value().data, res_bilinear.value().data);
}

TEST(VisionPreprocessingTest, AreaAveraging_NonIntegerScaling24bpp) {
    // Downscaling 15x9 to 4x3 in 24bpp format
    auto bmp24 = createTestBitmap24(15, 9, 80, 120, 160);
    auto res = BmpTool::resizeAreaAveraging(bmp24, 4, 3);
    ASSERT_TRUE(res.isSuccess());
    EXPECT_EQ(res.value().w, 4u);
    EXPECT_EQ(res.value().h, 3u);
    EXPECT_EQ(res.value().bpp, 24u);
    EXPECT_EQ(res.value().data[0], 80);
    EXPECT_EQ(res.value().data[1], 120);
    EXPECT_EQ(res.value().data[2], 160);
}

TEST(VisionPreprocessingTest, AreaAveraging_ParameterValidation) {
    auto bmp = createTestBitmap32(8, 8, 50, 50, 50);

    EXPECT_TRUE(BmpTool::resizeAreaAveraging(bmp, 0, 4).isError());
    EXPECT_TRUE(BmpTool::resizeAreaAveraging(bmp, 4, 0).isError());
    EXPECT_TRUE(BmpTool::resizeAreaAveraging(bmp, BmpTool::SafeMath::MAX_SAFE_DIMENSION + 1, 4).isError());

    auto zero_w = bmp;
    zero_w.w = 0;
    EXPECT_TRUE(BmpTool::resizeAreaAveraging(zero_w, 4, 4).isError());
}

// ===========================================================================
// Core Engine / Legacy API Tests
// ===========================================================================

TEST(VisionPreprocessingTest, LegacyEngine_ResizeBilinear) {
    Matrix::Matrix<Pixel> mat(4, 4);
    Pixel blue_pixel = {200, 100, 50, 255};
    for (int y = 0; y < 4; ++y) for (int x = 0; x < 4; ++x) mat[y][x] = blue_pixel;

    Bitmap::File bmp = CreateBitmapFromMatrix(mat);
    ASSERT_TRUE(bmp.IsValid());

    Bitmap::File resized = ResizeBilinearImage(bmp, 2, 2);
    ASSERT_TRUE(resized.IsValid());
    Matrix::Matrix<Pixel> out_mat = CreateMatrixFromBitmap(resized);
    ASSERT_EQ(out_mat.rows(), 2);
    ASSERT_EQ(out_mat.cols(), 2);
    EXPECT_EQ(out_mat[0][0].blue, 200);
    EXPECT_EQ(out_mat[0][0].green, 100);
    EXPECT_EQ(out_mat[0][0].red, 50);
}

TEST(VisionPreprocessingTest, LegacyEngine_ResizeAreaAveraging) {
    Matrix::Matrix<Pixel> mat(8, 8);
    Pixel green_pixel = {50, 220, 50, 255};
    for (int y = 0; y < 8; ++y) for (int x = 0; x < 8; ++x) mat[y][x] = green_pixel;

    Bitmap::File bmp = CreateBitmapFromMatrix(mat);
    ASSERT_TRUE(bmp.IsValid());

    Bitmap::File resized = ResizeAreaAveragingImage(bmp, 2, 2);
    ASSERT_TRUE(resized.IsValid());
    Matrix::Matrix<Pixel> out_mat = CreateMatrixFromBitmap(resized);
    ASSERT_EQ(out_mat.rows(), 2);
    ASSERT_EQ(out_mat.cols(), 2);
    EXPECT_EQ(out_mat[0][0].blue, 50);
    EXPECT_EQ(out_mat[0][0].green, 220);
    EXPECT_EQ(out_mat[0][0].red, 50);
}
