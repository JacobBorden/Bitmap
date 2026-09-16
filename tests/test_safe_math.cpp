#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>
#include <span>
#include <cmath>
#include "../src/safe_math.hpp"
#include "../../include/bitmap.hpp"
#include "../src/bitmapfile/bitmap_file.h"

using namespace BmpTool;

TEST(SafeMathTest, MultiplyUint32) {
    uint32_t out = 0;
    EXPECT_TRUE(SafeMath::multiply(10u, 20u, out));
    EXPECT_EQ(out, 200u);

    EXPECT_TRUE(SafeMath::multiply(0u, 50000u, out));
    EXPECT_EQ(out, 0u);

    // Overflow check
    EXPECT_FALSE(SafeMath::multiply(std::numeric_limits<uint32_t>::max(), 2u, out));
    EXPECT_FALSE(SafeMath::multiply(65536u, 65536u, out)); // 2^32 wraps uint32_t
}

TEST(SafeMathTest, MultiplySizeT) {
    size_t out = 0;
    EXPECT_TRUE(SafeMath::multiply(static_cast<size_t>(100), static_cast<size_t>(200), out));
    EXPECT_EQ(out, 20000u);

    EXPECT_TRUE(SafeMath::multiply(static_cast<size_t>(0), static_cast<size_t>(100), out));
    EXPECT_EQ(out, 0u);

    // Overflow check
    EXPECT_FALSE(SafeMath::multiply(std::numeric_limits<size_t>::max(), static_cast<size_t>(2), out));
}

TEST(SafeMathTest, AddUint32) {
    uint32_t out = 0;
    EXPECT_TRUE(SafeMath::add(100u, 200u, out));
    EXPECT_EQ(out, 300u);

    EXPECT_FALSE(SafeMath::add(std::numeric_limits<uint32_t>::max(), 1u, out));
    EXPECT_FALSE(SafeMath::add(std::numeric_limits<uint32_t>::max() - 5u, 10u, out));
}

TEST(SafeMathTest, AddSizeT) {
    size_t out = 0;
    EXPECT_TRUE(SafeMath::add(static_cast<size_t>(100), static_cast<size_t>(200), out));
    EXPECT_EQ(out, 300u);

    EXPECT_FALSE(SafeMath::add(std::numeric_limits<size_t>::max(), static_cast<size_t>(1), out));
}

TEST(SafeMathTest, GetSafeAbsoluteHeight) {
    uint32_t out = 0;
    EXPECT_TRUE(SafeMath::getSafeAbsoluteHeight(100, out));
    EXPECT_EQ(out, 100u);

    // Negative height (top-down BMP)
    EXPECT_TRUE(SafeMath::getSafeAbsoluteHeight(-100, out));
    EXPECT_EQ(out, 100u);

    // Zero height is invalid
    EXPECT_FALSE(SafeMath::getSafeAbsoluteHeight(0, out));

    // INT32_MIN overflow hazard
    EXPECT_FALSE(SafeMath::getSafeAbsoluteHeight(std::numeric_limits<int32_t>::min(), out));

    // Greater than MAX_SAFE_DIMENSION
    EXPECT_FALSE(SafeMath::getSafeAbsoluteHeight(SafeMath::MAX_SAFE_DIMENSION + 1, out));
    EXPECT_FALSE(SafeMath::getSafeAbsoluteHeight(-static_cast<int32_t>(SafeMath::MAX_SAFE_DIMENSION + 1), out));
}

TEST(SafeMathTest, ComputeRowStride) {
    uint32_t stride = 0;

    // 24 bpp: 1 pixel = 3 bytes -> aligned to 4
    EXPECT_TRUE(SafeMath::computeRowStride(1, 24, stride));
    EXPECT_EQ(stride, 4u);

    // 24 bpp: 2 pixels = 6 bytes -> aligned to 8
    EXPECT_TRUE(SafeMath::computeRowStride(2, 24, stride));
    EXPECT_EQ(stride, 8u);

    // 24 bpp: 3 pixels = 9 bytes -> aligned to 12
    EXPECT_TRUE(SafeMath::computeRowStride(3, 24, stride));
    EXPECT_EQ(stride, 12u);

    // 24 bpp: 4 pixels = 12 bytes -> aligned to 12
    EXPECT_TRUE(SafeMath::computeRowStride(4, 24, stride));
    EXPECT_EQ(stride, 12u);

    // 32 bpp: always multiple of 4
    EXPECT_TRUE(SafeMath::computeRowStride(1, 32, stride));
    EXPECT_EQ(stride, 4u);

    EXPECT_TRUE(SafeMath::computeRowStride(5, 32, stride));
    EXPECT_EQ(stride, 20u);

    // Invalid parameters
    EXPECT_FALSE(SafeMath::computeRowStride(0, 24, stride));
    EXPECT_FALSE(SafeMath::computeRowStride(10, 16, stride)); // unsupported bpp
    EXPECT_FALSE(SafeMath::computeRowStride(10, 8, stride));  // unsupported bpp
    EXPECT_FALSE(SafeMath::computeRowStride(SafeMath::MAX_SAFE_DIMENSION + 1, 32, stride));
}

TEST(SafeMathTest, ComputePixelDataSize) {
    uint32_t size = 0;
    EXPECT_TRUE(SafeMath::computePixelDataSize(10, 10, 32, size));
    EXPECT_EQ(size, 400u);

    // Zero dimensions
    EXPECT_FALSE(SafeMath::computePixelDataSize(0, 10, 32, size));
    EXPECT_FALSE(SafeMath::computePixelDataSize(10, 0, 32, size));

    // Exceeding dimension limits
    EXPECT_FALSE(SafeMath::computePixelDataSize(SafeMath::MAX_SAFE_DIMENSION + 1, 10, 32, size));
    EXPECT_FALSE(SafeMath::computePixelDataSize(10, SafeMath::MAX_SAFE_DIMENSION + 1, 32, size));

    // Exceeding memory limits (e.g. 65536 * 65536 * 4 = 16 GB > MAX_SAFE_IMAGE_BYTES)
    EXPECT_FALSE(SafeMath::computePixelDataSize(SafeMath::MAX_SAFE_DIMENSION, SafeMath::MAX_SAFE_DIMENSION, 32, size));
}

TEST(SecurityTest, MaliciousBmpMinIntHeight) {
    BITMAPFILEHEADER bfh = {};
    bfh.bfType = 0x4D42;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    BITMAPINFOHEADER bih = {};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = 10;
    bih.biHeight = std::numeric_limits<int32_t>::min(); // Overflow hazard
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = 0;

    std::vector<uint8_t> buffer(sizeof(bfh) + sizeof(bih));
    std::memcpy(buffer.data(), &bfh, sizeof(bfh));
    std::memcpy(buffer.data() + sizeof(bfh), &bih, sizeof(bih));

    auto result = load(std::span<const uint8_t>(buffer.data(), buffer.size()));
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), BitmapError::InvalidImageHeader);
}

TEST(SecurityTest, MaliciousBmpExcessiveDimensions) {
    BITMAPFILEHEADER bfh = {};
    bfh.bfType = 0x4D42;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    BITMAPINFOHEADER bih = {};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = SafeMath::MAX_SAFE_DIMENSION + 1; // 64K + 1
    bih.biHeight = 10;
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = 0;

    std::vector<uint8_t> buffer(sizeof(bfh) + sizeof(bih));
    std::memcpy(buffer.data(), &bfh, sizeof(bfh));
    std::memcpy(buffer.data() + sizeof(bfh), &bih, sizeof(bih));

    auto result = load(std::span<const uint8_t>(buffer.data(), buffer.size()));
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), BitmapError::ExceedsMaxDimensions);
}

TEST(SecurityTest, MaliciousBmpOffsetOverflow) {
    BITMAPFILEHEADER bfh = {};
    bfh.bfType = 0x4D42;
    bfh.bfOffBits = 0xFFFFFF00; // Near UINT32_MAX
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    BITMAPINFOHEADER bih = {};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = 10;
    bih.biHeight = 10;
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = 0;

    std::vector<uint8_t> buffer(sizeof(bfh) + sizeof(bih));
    std::memcpy(buffer.data(), &bfh, sizeof(bfh));
    std::memcpy(buffer.data() + sizeof(bfh), &bih, sizeof(bih));

    auto result = load(std::span<const uint8_t>(buffer.data(), buffer.size()));
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), BitmapError::InvalidFileHeader);
}

TEST(SecurityTest, TruncatedPayloadDetected) {
    BITMAPFILEHEADER bfh = {};
    bfh.bfType = 0x4D42;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 40000;

    BITMAPINFOHEADER bih = {};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = 100;
    bih.biHeight = 100;
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = 0;

    // Buffer has headers plus only 64 bytes instead of 40,000 bytes
    std::vector<uint8_t> buffer(sizeof(bfh) + sizeof(bih) + 64, 0);
    std::memcpy(buffer.data(), &bfh, sizeof(bfh));
    std::memcpy(buffer.data() + sizeof(bfh), &bih, sizeof(bih));

    auto result = load(std::span<const uint8_t>(buffer.data(), buffer.size()));
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), BitmapError::PayloadTruncated);
}

TEST(SecurityTest, TruncatedClaimedBiSizeImage) {
    BITMAPFILEHEADER bfh = {};
    bfh.bfType = 0x4D42;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 2000;

    BITMAPINFOHEADER bih = {};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = 10;
    bih.biHeight = 10;
    bih.biPlanes = 1;
    bih.biBitCount = 32; // expected 400 bytes
    bih.biCompression = 0;
    bih.biSizeImage = 2000; // claims 2000 bytes

    // Buffer only provides 450 bytes (covers 400 bytes expected, but truncated relative to claimed 2000)
    std::vector<uint8_t> buffer(sizeof(bfh) + sizeof(bih) + 450, 0);
    std::memcpy(buffer.data(), &bfh, sizeof(bfh));
    std::memcpy(buffer.data() + sizeof(bfh), &bih, sizeof(bih));

    auto result = load(std::span<const uint8_t>(buffer.data(), buffer.size()));
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), BitmapError::PayloadTruncated);
}

TEST(SecurityTest, UnsupportedCompressionCode) {
    BITMAPFILEHEADER bfh = {};
    bfh.bfType = 0x4D42;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 400;

    BITMAPINFOHEADER bih = {};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = 10;
    bih.biHeight = 10;
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = 1; // BI_RLE8 (unsupported compression code)

    std::vector<uint8_t> buffer(sizeof(bfh) + sizeof(bih) + 400, 0);
    std::memcpy(buffer.data(), &bfh, sizeof(bfh));
    std::memcpy(buffer.data() + sizeof(bfh), &bih, sizeof(bih));

    auto result = load(std::span<const uint8_t>(buffer.data(), buffer.size()));
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), BitmapError::UnsupportedCompression);
}

TEST(SecurityTest, InvalidHeaderBiSizeTooSmall) {
    BITMAPFILEHEADER bfh = {};
    bfh.bfType = 0x4D42;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 400;

    BITMAPINFOHEADER bih = {};
    bih.biSize = 20; // Invalid: must be at least sizeof(BITMAPINFOHEADER) (40)
    bih.biWidth = 10;
    bih.biHeight = 10;
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = 0;

    std::vector<uint8_t> buffer(sizeof(bfh) + sizeof(bih) + 400, 0);
    std::memcpy(buffer.data(), &bfh, sizeof(bfh));
    std::memcpy(buffer.data() + sizeof(bfh), &bih, sizeof(bih));

    auto result = load(std::span<const uint8_t>(buffer.data(), buffer.size()));
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), BitmapError::InvalidImageHeader);
}

TEST(SecurityTest, InvalidHeaderPlanesNotOne) {
    BITMAPFILEHEADER bfh = {};
    bfh.bfType = 0x4D42;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 400;

    BITMAPINFOHEADER bih = {};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = 10;
    bih.biHeight = 10;
    bih.biPlanes = 2; // Invalid: must be 1
    bih.biBitCount = 32;
    bih.biCompression = 0;

    std::vector<uint8_t> buffer(sizeof(bfh) + sizeof(bih) + 400, 0);
    std::memcpy(buffer.data(), &bfh, sizeof(bfh));
    std::memcpy(buffer.data() + sizeof(bfh), &bih, sizeof(bih));

    auto result = load(std::span<const uint8_t>(buffer.data(), buffer.size()));
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), BitmapError::InvalidImageHeader);
}

TEST(SecurityTest, InconsistentBiSizeImageTooSmall) {
    BITMAPFILEHEADER bfh = {};
    bfh.bfType = 0x4D42;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 400;

    BITMAPINFOHEADER bih = {};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = 10;
    bih.biHeight = 10;
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = 0;
    bih.biSizeImage = 50; // Declares 50 bytes, but dimensions require 400 bytes!

    std::vector<uint8_t> buffer(sizeof(bfh) + sizeof(bih) + 400, 0);
    std::memcpy(buffer.data(), &bfh, sizeof(bfh));
    std::memcpy(buffer.data() + sizeof(bfh), &bih, sizeof(bih));

    auto result = load(std::span<const uint8_t>(buffer.data(), buffer.size()));
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), BitmapError::InvalidImageHeader);
}

TEST(SecurityTest, OffsetBitsSmallerThanHeaders) {
    BITMAPFILEHEADER bfh = {};
    bfh.bfType = 0x4D42;
    bfh.bfOffBits = 30; // Smaller than sizeof(BITMAPFILEHEADER) + biSize (54)
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 400;

    BITMAPINFOHEADER bih = {};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = 10;
    bih.biHeight = 10;
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = 0;

    std::vector<uint8_t> buffer(sizeof(bfh) + sizeof(bih) + 400, 0);
    std::memcpy(buffer.data(), &bfh, sizeof(bfh));
    std::memcpy(buffer.data() + sizeof(bfh), &bih, sizeof(bih));

    auto result = load(std::span<const uint8_t>(buffer.data(), buffer.size()));
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), BitmapError::InvalidFileHeader);
}

TEST(SecurityTest, MaliciousBmpExcessiveHeight) {
    BITMAPFILEHEADER bfh = {};
    bfh.bfType = 0x4D42;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 400;

    BITMAPINFOHEADER bih = {};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = 10;
    bih.biHeight = SafeMath::MAX_SAFE_DIMENSION + 1; // 64K + 1
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = 0;

    std::vector<uint8_t> buffer(sizeof(bfh) + sizeof(bih) + 400, 0);
    std::memcpy(buffer.data(), &bfh, sizeof(bfh));
    std::memcpy(buffer.data() + sizeof(bfh), &bih, sizeof(bih));

    auto result = load(std::span<const uint8_t>(buffer.data(), buffer.size()));
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), BitmapError::ExceedsMaxDimensions);

    // Negative excessive height (top-down)
    bih.biHeight = -static_cast<int32_t>(SafeMath::MAX_SAFE_DIMENSION + 1);
    std::memcpy(buffer.data() + sizeof(bfh), &bih, sizeof(bih));
    auto result_neg = load(std::span<const uint8_t>(buffer.data(), buffer.size()));
    EXPECT_TRUE(result_neg.isError());
    EXPECT_EQ(result_neg.error(), BitmapError::ExceedsMaxDimensions);
}

TEST(SecurityTest, MaliciousBmpInvalidColorDepth) {
    BITMAPFILEHEADER bfh = {};
    bfh.bfType = 0x4D42;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 400;

    BITMAPINFOHEADER bih = {};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = 10;
    bih.biHeight = 10;
    bih.biPlanes = 1;
    bih.biCompression = 0;

    std::vector<uint8_t> buffer(sizeof(bfh) + sizeof(bih) + 400, 0);
    std::memcpy(buffer.data(), &bfh, sizeof(bfh));

    // Test unsupported bit depths: 1, 4, 8, 16, 48
    uint16_t invalid_bpps[] = {1, 4, 8, 16, 48};
    for (uint16_t bpp : invalid_bpps) {
        bih.biBitCount = bpp;
        std::memcpy(buffer.data() + sizeof(bfh), &bih, sizeof(bih));
        auto result = load(std::span<const uint8_t>(buffer.data(), buffer.size()));
        EXPECT_TRUE(result.isError());
        EXPECT_EQ(result.error(), BitmapError::InvalidColorDepth);
    }
}

TEST(SafeMathTest, ClampHelper) {
    EXPECT_EQ(SafeMath::clamp(10, 0, 20), 10);
    EXPECT_EQ(SafeMath::clamp(-5, 0, 20), 0);
    EXPECT_EQ(SafeMath::clamp(25, 0, 20), 20);
    EXPECT_EQ(SafeMath::clamp(100, 1, SafeMath::MAX_SAFE_SCALE_FACTOR), 100);
    EXPECT_EQ(SafeMath::clamp(1000, 1, SafeMath::MAX_SAFE_SCALE_FACTOR), SafeMath::MAX_SAFE_SCALE_FACTOR);
    EXPECT_EQ(SafeMath::clamp(0, 1, SafeMath::MAX_SAFE_SCALE_FACTOR), 1);
    EXPECT_EQ(SafeMath::clamp(100, 0, SafeMath::MAX_SAFE_BLUR_RADIUS), SafeMath::MAX_SAFE_BLUR_RADIUS);
}

TEST(SecurityTest, DefensiveParameterClampingBoxBlur) {
    BmpTool::Bitmap test_bmp;
    test_bmp.w = 4;
    test_bmp.h = 4;
    test_bmp.bpp = 32;
    test_bmp.data.assign(4 * 4 * 4, 128);

    // Extreme blur radius should be safely clamped to MAX_SAFE_BLUR_RADIUS without hanging or crashing
    auto result = applyBoxBlur(test_bmp, 10000);
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(result.value().w, 4u);
    EXPECT_EQ(result.value().h, 4u);
}

TEST(SecurityTest, DefensiveParameterClampingShrink) {
    BmpTool::Bitmap test_bmp;
    test_bmp.w = 512;
    test_bmp.h = 512;
    test_bmp.bpp = 32;
    test_bmp.data.assign(512 * 512 * 4, 128);

    // Scale factor 5000 is clamped to MAX_SAFE_SCALE_FACTOR (256).
    // Resulting dimensions: 512 / 256 = 2.
    auto result = shrink(test_bmp, 5000);
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(result.value().w, 2u);
    EXPECT_EQ(result.value().h, 2u);
}

TEST(SecurityTest, NonFiniteFloatRejection) {
    BmpTool::Bitmap test_bmp;
    test_bmp.w = 2;
    test_bmp.h = 2;
    test_bmp.bpp = 32;
    test_bmp.data.assign(2 * 2 * 4, 100);

    const float nan_val = std::numeric_limits<float>::quiet_NaN();
    const float inf_val = std::numeric_limits<float>::infinity();
    const float neg_val = -0.5f;

    // Brightness
    EXPECT_TRUE(changeBrightness(test_bmp, nan_val).isError());
    EXPECT_EQ(changeBrightness(test_bmp, nan_val).error(), BitmapError::InvalidImageData);
    EXPECT_TRUE(changeBrightness(test_bmp, inf_val).isError());
    EXPECT_EQ(changeBrightness(test_bmp, inf_val).error(), BitmapError::InvalidImageData);
    EXPECT_TRUE(changeBrightness(test_bmp, neg_val).isError());
    EXPECT_EQ(changeBrightness(test_bmp, neg_val).error(), BitmapError::InvalidImageData);

    // Contrast
    EXPECT_TRUE(changeContrast(test_bmp, nan_val).isError());
    EXPECT_EQ(changeContrast(test_bmp, nan_val).error(), BitmapError::InvalidImageData);
    EXPECT_TRUE(changeContrast(test_bmp, inf_val).isError());
    EXPECT_TRUE(changeContrast(test_bmp, neg_val).isError());

    // Saturation
    EXPECT_TRUE(changeSaturation(test_bmp, nan_val).isError());
    EXPECT_EQ(changeSaturation(test_bmp, nan_val).error(), BitmapError::InvalidImageData);
    EXPECT_TRUE(changeSaturation(test_bmp, inf_val).isError());
    EXPECT_TRUE(changeSaturation(test_bmp, neg_val).isError());

    // Luminance variants
    EXPECT_TRUE(changeLuminanceBlue(test_bmp, nan_val).isError());
    EXPECT_TRUE(changeLuminanceBlue(test_bmp, inf_val).isError());
    EXPECT_TRUE(changeLuminanceBlue(test_bmp, neg_val).isError());
}

