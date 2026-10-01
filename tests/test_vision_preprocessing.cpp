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

// ===========================================================================
// Day 13: Letterbox Padding & Aspect-Ratio Preservation Tests
// ===========================================================================

TEST(VisionPreprocessingTest, Letterbox_SquareToSquare) {
    auto bmp = createTestBitmap32(10, 10, 255, 0, 0, 255);
    BmpTool::LetterboxMetadata meta;
    auto res = BmpTool::letterbox(bmp, 20, 20, BmpTool::PadColor{114, 114, 114, 255}, &meta);
    ASSERT_TRUE(res.isSuccess());
    const auto& lb = res.value();

    EXPECT_EQ(lb.w, 20u);
    EXPECT_EQ(lb.h, 20u);
    EXPECT_EQ(meta.targetWidth, 20u);
    EXPECT_EQ(meta.targetHeight, 20u);
    EXPECT_EQ(meta.scaledWidth, 20u);
    EXPECT_EQ(meta.scaledHeight, 20u);
    EXPECT_EQ(meta.padLeft, 0u);
    EXPECT_EQ(meta.padTop, 0u);
    EXPECT_FLOAT_EQ(meta.scaleRatio, 2.0f);

    // Entire canvas must be red (scaled 1:1 without letterbox bars)
    for (size_t i = 0; i < lb.data.size(); i += 4) {
        EXPECT_EQ(lb.data[i], 255);
        EXPECT_EQ(lb.data[i + 1], 0);
        EXPECT_EQ(lb.data[i + 2], 0);
    }
}

TEST(VisionPreprocessingTest, Letterbox_HorizontalPanoramicToSquare) {
    // 20x10 pure blue image letterboxed to 20x20
    // Aspect ratio 2:1 -> Scale ratio = min(20/20, 20/10) = 1.0
    // scaledW = 20, scaledH = 10 -> padLeft = 0, padTop = 5
    auto bmp = createTestBitmap32(20, 10, 0, 0, 255, 255);
    BmpTool::LetterboxMetadata meta;
    auto res = BmpTool::letterbox(bmp, 20, 20, BmpTool::PadColor{114, 114, 114, 255}, &meta);
    ASSERT_TRUE(res.isSuccess());
    const auto& lb = res.value();

    EXPECT_EQ(lb.w, 20u);
    EXPECT_EQ(lb.h, 20u);
    EXPECT_EQ(meta.scaledWidth, 20u);
    EXPECT_EQ(meta.scaledHeight, 10u);
    EXPECT_EQ(meta.padLeft, 0u);
    EXPECT_EQ(meta.padTop, 5u);
    EXPECT_FLOAT_EQ(meta.scaleRatio, 1.0f);

    // Top padding rows (y in [0, 4]) must be neutral gray (114, 114, 114)
    for (uint32_t y = 0; y < 5; ++y) {
        for (uint32_t x = 0; x < 20; ++x) {
            size_t idx = (y * 20 + x) * 4;
            EXPECT_EQ(lb.data[idx], 114) << "Mismatch at pad top row " << y;
            EXPECT_EQ(lb.data[idx + 1], 114);
            EXPECT_EQ(lb.data[idx + 2], 114);
        }
    }

    // Centered image rows (y in [5, 14]) must be blue (0, 0, 255)
    for (uint32_t y = 5; y < 15; ++y) {
        for (uint32_t x = 0; x < 20; ++x) {
            size_t idx = (y * 20 + x) * 4;
            EXPECT_EQ(lb.data[idx], 0) << "Mismatch at image row " << y;
            EXPECT_EQ(lb.data[idx + 1], 0);
            EXPECT_EQ(lb.data[idx + 2], 255);
        }
    }

    // Bottom padding rows (y in [15, 19]) must be neutral gray (114, 114, 114)
    for (uint32_t y = 15; y < 20; ++y) {
        for (uint32_t x = 0; x < 20; ++x) {
            size_t idx = (y * 20 + x) * 4;
            EXPECT_EQ(lb.data[idx], 114) << "Mismatch at pad bottom row " << y;
            EXPECT_EQ(lb.data[idx + 1], 114);
            EXPECT_EQ(lb.data[idx + 2], 114);
        }
    }
}

TEST(VisionPreprocessingTest, Letterbox_VerticalPortraitToSquare) {
    // 10x20 pure green image letterboxed to 20x20
    // Aspect ratio 1:2 -> Scale ratio = min(20/10, 20/20) = 1.0
    // scaledW = 10, scaledH = 20 -> padLeft = 5, padTop = 0
    auto bmp = createTestBitmap32(10, 20, 0, 255, 0, 255);
    BmpTool::LetterboxMetadata meta;
    auto res = BmpTool::letterbox(bmp, 20, 20, BmpTool::PadColor{50, 50, 50, 255}, &meta);
    ASSERT_TRUE(res.isSuccess());
    const auto& lb = res.value();

    EXPECT_EQ(lb.w, 20u);
    EXPECT_EQ(lb.h, 20u);
    EXPECT_EQ(meta.scaledWidth, 10u);
    EXPECT_EQ(meta.scaledHeight, 20u);
    EXPECT_EQ(meta.padLeft, 5u);
    EXPECT_EQ(meta.padTop, 0u);

    // Left padding columns (x in [0, 4]) must be (50, 50, 50)
    for (uint32_t y = 0; y < 20; ++y) {
        for (uint32_t x = 0; x < 5; ++x) {
            size_t idx = (y * 20 + x) * 4;
            EXPECT_EQ(lb.data[idx], 50);
            EXPECT_EQ(lb.data[idx + 1], 50);
            EXPECT_EQ(lb.data[idx + 2], 50);
        }
    }

    // Centered columns (x in [5, 14]) must be green (0, 255, 0)
    for (uint32_t y = 0; y < 20; ++y) {
        for (uint32_t x = 5; x < 15; ++x) {
            size_t idx = (y * 20 + x) * 4;
            EXPECT_EQ(lb.data[idx], 0);
            EXPECT_EQ(lb.data[idx + 1], 255);
            EXPECT_EQ(lb.data[idx + 2], 0);
        }
    }

    // Right padding columns (x in [15, 19]) must be (50, 50, 50)
    for (uint32_t y = 0; y < 20; ++y) {
        for (uint32_t x = 15; x < 20; ++x) {
            size_t idx = (y * 20 + x) * 4;
            EXPECT_EQ(lb.data[idx], 50);
            EXPECT_EQ(lb.data[idx + 1], 50);
            EXPECT_EQ(lb.data[idx + 2], 50);
        }
    }
}

TEST(VisionPreprocessingTest, Letterbox_BoundingBoxCoordinateInversion) {
    // Simulate neural object detection mapping:
    // Raw camera: 1920x1080 -> Letterbox model input: 640x640
    auto bmp = createTestBitmap32(1920, 1080, 100, 100, 100);
    BmpTool::LetterboxMetadata meta;
    auto res = BmpTool::letterbox(bmp, 640, 640, BmpTool::PadColor{114, 114, 114, 255}, &meta);
    ASSERT_TRUE(res.isSuccess());

    // Scale ratio = 640 / 1920 = 1/3
    EXPECT_NEAR(meta.scaleRatio, 640.0f / 1920.0f, 1e-4f);
    EXPECT_EQ(meta.scaledWidth, 640u);
    EXPECT_EQ(meta.scaledHeight, 360u);
    EXPECT_EQ(meta.padLeft, 0u);
    EXPECT_EQ(meta.padTop, 140u); // (640 - 360) / 2 = 140

    // Bounding box on model prediction: [padLeft, padTop, padLeft + scaledWidth, padTop + scaledHeight]
    float box_x1 = static_cast<float>(meta.padLeft);
    float box_y1 = static_cast<float>(meta.padTop);
    float box_x2 = static_cast<float>(meta.padLeft + meta.scaledWidth);
    float box_y2 = static_cast<float>(meta.padTop + meta.scaledHeight);

    // Map back to original image space
    float raw_x1 = (box_x1 - meta.padLeft) / meta.scaleRatio;
    float raw_y1 = (box_y1 - meta.padTop) / meta.scaleRatio;
    float raw_x2 = (box_x2 - meta.padLeft) / meta.scaleRatio;
    float raw_y2 = (box_y2 - meta.padTop) / meta.scaleRatio;

    EXPECT_NEAR(raw_x1, 0.0f, 0.5f);
    EXPECT_NEAR(raw_y1, 0.0f, 0.5f);
    EXPECT_NEAR(raw_x2, 1920.0f, 0.5f);
    EXPECT_NEAR(raw_y2, 1080.0f, 0.5f);
}

TEST(VisionPreprocessingTest, Letterbox_24bppSupport) {
    auto bmp24 = createTestBitmap24(8, 4, 10, 20, 30);
    BmpTool::LetterboxMetadata meta;
    auto res = BmpTool::letterbox(bmp24, 8, 8, BmpTool::PadColor{114, 114, 114, 255}, &meta);
    ASSERT_TRUE(res.isSuccess());
    const auto& lb = res.value();

    EXPECT_EQ(lb.bpp, 24u);
    EXPECT_EQ(lb.w, 8u);
    EXPECT_EQ(lb.h, 8u);
    EXPECT_EQ(meta.padTop, 2u);

    // Row 0 is pad
    EXPECT_EQ(lb.data[0], 114);
    EXPECT_EQ(lb.data[1], 114);
    EXPECT_EQ(lb.data[2], 114);

    // Row 2 is image
    size_t img_idx = 2 * 8 * 3;
    EXPECT_EQ(lb.data[img_idx], 10);
    EXPECT_EQ(lb.data[img_idx + 1], 20);
    EXPECT_EQ(lb.data[img_idx + 2], 30);
}

TEST(VisionPreprocessingTest, Letterbox_ParameterValidation) {
    auto bmp = createTestBitmap32(4, 4, 10, 20, 30);
    EXPECT_TRUE(BmpTool::letterbox(bmp, 0, 4).isError());
    EXPECT_TRUE(BmpTool::letterbox(bmp, 4, 0).isError());
    EXPECT_TRUE(BmpTool::letterbox(bmp, BmpTool::SafeMath::MAX_SAFE_DIMENSION + 1, 4).isError());

    auto zero_bmp = bmp;
    zero_bmp.w = 0;
    EXPECT_TRUE(BmpTool::letterbox(zero_bmp, 4, 4).isError());
}

TEST(VisionPreprocessingTest, LegacyEngine_LetterboxImage) {
    Matrix::Matrix<Pixel> mat(4, 2);
    Pixel white_pixel = {255, 255, 255, 255};
    for (int y = 0; y < 4; ++y) for (int x = 0; x < 2; ++x) mat[y][x] = white_pixel;

    Bitmap::File bmp = CreateBitmapFromMatrix(mat);
    ASSERT_TRUE(bmp.IsValid());

    Pixel gray_pad = {114, 114, 114, 255};
    Bitmap::File lb_file = LetterboxImage(bmp, 4, 4, gray_pad);
    ASSERT_TRUE(lb_file.IsValid());
    Matrix::Matrix<Pixel> lb_mat = CreateMatrixFromBitmap(lb_file);
    ASSERT_EQ(lb_mat.rows(), 4);
    ASSERT_EQ(lb_mat.cols(), 4);

    // Height = 4, Width = 2 -> r = min(4/2, 4/4) = 1.0. scaledW = 2, scaledH = 4.
    // padLeft = 1, padTop = 0
    EXPECT_EQ(lb_mat[0][0].red, 114); // pad
    EXPECT_EQ(lb_mat[0][1].red, 255); // image
    EXPECT_EQ(lb_mat[0][2].red, 255); // image
    EXPECT_EQ(lb_mat[0][3].red, 114); // pad
}

TEST(VisionPreprocessingTest, Crop_ValidSubregion) {
    // 10x10 image with unique RGBA pixel values based on coordinates
    BmpTool::Bitmap bmp;
    bmp.w = 10;
    bmp.h = 10;
    bmp.bpp = 32;
    bmp.data.resize(10 * 10 * 4);
    for (uint32_t y = 0; y < 10; ++y) {
        for (uint32_t x = 0; x < 10; ++x) {
            size_t idx = (y * 10 + x) * 4;
            bmp.data[idx + 0] = static_cast<uint8_t>(x * 10);
            bmp.data[idx + 1] = static_cast<uint8_t>(y * 10);
            bmp.data[idx + 2] = static_cast<uint8_t>(x + y);
            bmp.data[idx + 3] = 255;
        }
    }

    auto res = BmpTool::crop(bmp, 2, 3, 4, 5);
    ASSERT_TRUE(res.isSuccess());
    const auto& cropped = res.value();

    EXPECT_EQ(cropped.w, 4u);
    EXPECT_EQ(cropped.h, 5u);
    EXPECT_EQ(cropped.bpp, 32u);
    EXPECT_EQ(cropped.data.size(), 4u * 5u * 4u);

    for (uint32_t r = 0; r < 5; ++r) {
        for (uint32_t c = 0; c < 4; ++c) {
            size_t crop_idx = (r * 4 + c) * 4;
            uint32_t orig_x = 2 + c;
            uint32_t orig_y = 3 + r;
            EXPECT_EQ(cropped.data[crop_idx + 0], static_cast<uint8_t>(orig_x * 10));
            EXPECT_EQ(cropped.data[crop_idx + 1], static_cast<uint8_t>(orig_y * 10));
            EXPECT_EQ(cropped.data[crop_idx + 2], static_cast<uint8_t>(orig_x + orig_y));
            EXPECT_EQ(cropped.data[crop_idx + 3], 255);
        }
    }
}

TEST(VisionPreprocessingTest, Crop_OutOfBoundsRejection) {
    auto bmp = createTestBitmap32(10, 10, 50, 50, 50);

    // x + w > bitmap.w
    EXPECT_TRUE(BmpTool::crop(bmp, 8, 2, 4, 4).isError());
    // y + h > bitmap.h
    EXPECT_TRUE(BmpTool::crop(bmp, 2, 8, 4, 4).isError());
    // Zero width / height
    EXPECT_TRUE(BmpTool::crop(bmp, 2, 2, 0, 4).isError());
    EXPECT_TRUE(BmpTool::crop(bmp, 2, 2, 4, 0).isError());
    // Overflow
    EXPECT_TRUE(BmpTool::crop(bmp, UINT32_MAX, 0, 10, 10).isError());
}

TEST(VisionPreprocessingTest, Crop_24bppAnd32bpp) {
    auto bmp24 = createTestBitmap24(6, 6, 12, 34, 56);
    auto res24 = BmpTool::crop(bmp24, 1, 1, 3, 3);
    ASSERT_TRUE(res24.isSuccess());
    EXPECT_EQ(res24.value().w, 3u);
    EXPECT_EQ(res24.value().h, 3u);
    EXPECT_EQ(res24.value().bpp, 24u);
    EXPECT_EQ(res24.value().data.size(), 3u * 3u * 3u);
    EXPECT_EQ(res24.value().data[0], 12);
    EXPECT_EQ(res24.value().data[1], 34);
    EXPECT_EQ(res24.value().data[2], 56);
}

TEST(VisionPreprocessingTest, ExportPlanarFloat_ImageNetStandardization) {
    // 2x2 bitmap with known RGB values
    BmpTool::Bitmap bmp;
    bmp.w = 2;
    bmp.h = 2;
    bmp.bpp = 32;
    bmp.data = {
        255,   0, 128, 255,  // Pixel (0,0)
          0, 255,  64, 255,  // Pixel (1,0)
        128, 128, 255, 255,  // Pixel (0,1)
         50,  75, 100, 255   // Pixel (1,1)
    };

    std::vector<float> planar_buffer(3 * 2 * 2);
    auto res = BmpTool::exportPlanarFloat(bmp, planar_buffer, BmpTool::NormalizationParams::ImageNet());
    ASSERT_TRUE(res.isSuccess());

    // NCHW planes: R plane [0..3], G plane [4..7], B plane [8..11]
    const float* R = planar_buffer.data();
    const float* G = R + 4;
    const float* B = G + 4;

    // Check Pixel (0,0): R=255, G=0, B=128
    // R: (255/255.0 - 0.485) / 0.229 = (1.0 - 0.485) / 0.229 = 0.515 / 0.229
    EXPECT_NEAR(R[0], (1.0f - 0.485f) / 0.229f, 1e-4f);
    // G: (0/255.0 - 0.456) / 0.224 = -0.456 / 0.224
    EXPECT_NEAR(G[0], (0.0f - 0.456f) / 0.224f, 1e-4f);
    // B: (128/255.0 - 0.406) / 0.225
    EXPECT_NEAR(B[0], ((128.0f / 255.0f) - 0.406f) / 0.225f, 1e-4f);

    // Check Pixel (1,0): R=0, G=255, B=64
    EXPECT_NEAR(R[1], (0.0f - 0.485f) / 0.229f, 1e-4f);
    EXPECT_NEAR(G[1], (1.0f - 0.456f) / 0.224f, 1e-4f);
    EXPECT_NEAR(B[1], ((64.0f / 255.0f) - 0.406f) / 0.225f, 1e-4f);
}

TEST(VisionPreprocessingTest, ExportInterleavedFloat_NHWCLayout) {
    // 2x1 bitmap with MinusOneToOne normalization
    BmpTool::Bitmap bmp;
    bmp.w = 2;
    bmp.h = 1;
    bmp.bpp = 24;
    bmp.data = {
          0, 255, 128,  // Pixel 0: R=0, G=255, B=128
        255,   0,  64   // Pixel 1: R=255, G=0, B=64
    };

    std::vector<float> nhwc_buffer(3 * 2 * 1);
    auto res = BmpTool::exportInterleavedFloat(bmp, nhwc_buffer, BmpTool::NormalizationParams::MinusOneToOne());
    ASSERT_TRUE(res.isSuccess());

    // Interleaved [H, W, C]: [R0, G0, B0, R1, G1, B1]
    // MinusOneToOne: (val / 255.0 - 0.5) / 0.5 = val / 127.5 - 1.0
    // Pixel 0:
    EXPECT_NEAR(nhwc_buffer[0], -1.0f, 1e-4f);
    EXPECT_NEAR(nhwc_buffer[1], 1.0f, 1e-4f);
    EXPECT_NEAR(nhwc_buffer[2], (128.0f / 127.5f) - 1.0f, 1e-4f);

    // Pixel 1:
    EXPECT_NEAR(nhwc_buffer[3], 1.0f, 1e-4f);
    EXPECT_NEAR(nhwc_buffer[4], -1.0f, 1e-4f);
    EXPECT_NEAR(nhwc_buffer[5], (64.0f / 127.5f) - 1.0f, 1e-4f);
}

TEST(VisionPreprocessingTest, ExportPlanarUint8_Deinterleave) {
    // 3x1 bitmap
    BmpTool::Bitmap bmp;
    bmp.w = 3;
    bmp.h = 1;
    bmp.bpp = 32;
    bmp.data = {
        10, 20, 30, 255,
        40, 50, 60, 255,
        70, 80, 90, 255
    };

    std::vector<uint8_t> planar_buffer(3 * 3 * 1);
    auto res = BmpTool::exportPlanarUint8(bmp, planar_buffer);
    ASSERT_TRUE(res.isSuccess());

    // Plane 0 (R): 10, 40, 70
    EXPECT_EQ(planar_buffer[0], 10);
    EXPECT_EQ(planar_buffer[1], 40);
    EXPECT_EQ(planar_buffer[2], 70);

    // Plane 1 (G): 20, 50, 80
    EXPECT_EQ(planar_buffer[3], 20);
    EXPECT_EQ(planar_buffer[4], 50);
    EXPECT_EQ(planar_buffer[5], 80);

    // Plane 2 (B): 30, 60, 90
    EXPECT_EQ(planar_buffer[6], 30);
    EXPECT_EQ(planar_buffer[7], 60);
    EXPECT_EQ(planar_buffer[8], 90);
}

TEST(VisionPreprocessingTest, Export_BufferTooSmall) {
    auto bmp = createTestBitmap32(4, 4, 10, 20, 30);
    // Required size is 4 * 4 * 3 = 48 elements
    std::vector<float> small_float_buf(40);
    std::vector<uint8_t> small_uint8_buf(40);

    auto res1 = BmpTool::exportPlanarFloat(bmp, small_float_buf);
    EXPECT_TRUE(res1.isError());
    EXPECT_EQ(res1.error(), BmpTool::BitmapError::OutputBufferTooSmall);

    auto res2 = BmpTool::exportInterleavedFloat(bmp, small_float_buf);
    EXPECT_TRUE(res2.isError());
    EXPECT_EQ(res2.error(), BmpTool::BitmapError::OutputBufferTooSmall);

    auto res3 = BmpTool::exportPlanarUint8(bmp, small_uint8_buf);
    EXPECT_TRUE(res3.isError());
    EXPECT_EQ(res3.error(), BmpTool::BitmapError::OutputBufferTooSmall);
}

TEST(VisionPreprocessingTest, LegacyEngine_CropImage) {
    Matrix::Matrix<Pixel> mat(4, 4);
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            mat[y][x] = Pixel{static_cast<BYTE>(x * 10), static_cast<BYTE>(y * 10), 0, 255};
        }
    }

    Bitmap::File bmp = CreateBitmapFromMatrix(mat);
    ASSERT_TRUE(bmp.IsValid());

    Bitmap::File cropped = CropImage(bmp, 1, 1, 2, 2);
    ASSERT_TRUE(cropped.IsValid());
    Matrix::Matrix<Pixel> crop_mat = CreateMatrixFromBitmap(cropped);
    ASSERT_EQ(crop_mat.rows(), 2);
    ASSERT_EQ(crop_mat.cols(), 2);

    EXPECT_EQ(crop_mat[0][0].blue, 10);
    EXPECT_EQ(crop_mat[0][0].green, 10);
    EXPECT_EQ(crop_mat[1][1].blue, 20);
    EXPECT_EQ(crop_mat[1][1].green, 20);

    // Invalid crop returns invalid Bitmap::File
    Bitmap::File invalid_crop = CropImage(bmp, 3, 3, 2, 2);
    EXPECT_FALSE(invalid_crop.IsValid());
}

