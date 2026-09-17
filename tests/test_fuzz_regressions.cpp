#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <span>
#include <limits>
#include "../include/bitmap.hpp"
#include "../src/bitmapfile/bitmap_file.h"

namespace {

// Helper to assemble valid 24bpp BMP bytes
std::vector<uint8_t> createValidBmp24bpp(int32_t width, int32_t height) {
    uint32_t abs_h = static_cast<uint32_t>(std::abs(height));
    uint32_t stride = ((static_cast<uint32_t>(width) * 3 + 3) / 4) * 4;
    uint32_t pixel_size = stride * abs_h;
    uint32_t total_size = 54 + pixel_size;

    std::vector<uint8_t> buf(total_size, 0);

    // BITMAPFILEHEADER
    buf[0] = 0x42; buf[1] = 0x4D; // 'BM'
    std::memcpy(&buf[2], &total_size, 4);
    uint32_t offbits = 54;
    std::memcpy(&buf[10], &offbits, 4);

    // BITMAPINFOHEADER
    uint32_t bisize = 40;
    std::memcpy(&buf[14], &bisize, 4);
    std::memcpy(&buf[18], &width, 4);
    std::memcpy(&buf[22], &height, 4);
    uint16_t planes = 1;
    std::memcpy(&buf[26], &planes, 2);
    uint16_t bpp = 24;
    std::memcpy(&buf[28], &bpp, 2);
    uint32_t compression = 0;
    std::memcpy(&buf[30], &compression, 4);
    std::memcpy(&buf[34], &pixel_size, 4);

    // Fill pixels with dummy color
    for (size_t i = 54; i < total_size; ++i) {
        buf[i] = static_cast<uint8_t>(i & 0xFF);
    }

    return buf;
}

// Helper to assemble valid 32bpp BMP bytes
std::vector<uint8_t> createValidBmp32bpp(int32_t width, int32_t height) {
    uint32_t abs_h = static_cast<uint32_t>(std::abs(height));
    uint32_t stride = static_cast<uint32_t>(width) * 4;
    uint32_t pixel_size = stride * abs_h;
    uint32_t total_size = 54 + pixel_size;

    std::vector<uint8_t> buf(total_size, 0);

    // BITMAPFILEHEADER
    buf[0] = 0x42; buf[1] = 0x4D; // 'BM'
    std::memcpy(&buf[2], &total_size, 4);
    uint32_t offbits = 54;
    std::memcpy(&buf[10], &offbits, 4);

    // BITMAPINFOHEADER
    uint32_t bisize = 40;
    std::memcpy(&buf[14], &bisize, 4);
    std::memcpy(&buf[18], &width, 4);
    std::memcpy(&buf[22], &height, 4);
    uint16_t planes = 1;
    std::memcpy(&buf[26], &planes, 2);
    uint16_t bpp = 32;
    std::memcpy(&buf[28], &bpp, 2);
    uint32_t compression = 0;
    std::memcpy(&buf[30], &compression, 4);
    std::memcpy(&buf[34], &pixel_size, 4);

    // Fill pixels with dummy color
    for (size_t i = 54; i < total_size; ++i) {
        buf[i] = static_cast<uint8_t>((i * 7) & 0xFF);
    }

    return buf;
}

} // namespace

TEST(FuzzRegressionTest, CorruptMagicBytes) {
    auto bmp = createValidBmp24bpp(2, 2);
    
    // Corrupt 'BM' to 'XX' -> NotABmp
    bmp[0] = 'X'; bmp[1] = 'X';
    auto res = BmpTool::load(std::span<const uint8_t>(bmp.data(), bmp.size()));
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error(), BmpTool::BitmapError::NotABmp);

    // Corrupt to null bytes -> NotABmp
    bmp[0] = 0; bmp[1] = 0;
    res = BmpTool::load(std::span<const uint8_t>(bmp.data(), bmp.size()));
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error(), BmpTool::BitmapError::NotABmp);
}

TEST(FuzzRegressionTest, CorruptInfoSize) {
    auto bmp = createValidBmp24bpp(2, 2);

    // biSize = 0
    uint32_t zero_size = 0;
    std::memcpy(&bmp[14], &zero_size, 4);
    auto res = BmpTool::load(std::span<const uint8_t>(bmp.data(), bmp.size()));
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidImageHeader);

    // biSize = 12 (OS/2 style header, unsupported)
    uint32_t os2_size = 12;
    std::memcpy(&bmp[14], &os2_size, 4);
    res = BmpTool::load(std::span<const uint8_t>(bmp.data(), bmp.size()));
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidImageHeader);

    // biSize = 108 (V4 header): with bfOffBits = 54, offbits < 14 + 108 -> InvalidFileHeader
    uint32_t v4_size = 108;
    std::memcpy(&bmp[14], &v4_size, 4);
    res = BmpTool::load(std::span<const uint8_t>(bmp.data(), bmp.size()));
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidFileHeader);
}

TEST(FuzzRegressionTest, CorruptPlanes) {
    auto bmp = createValidBmp24bpp(2, 2);

    // biPlanes = 0
    uint16_t planes_zero = 0;
    std::memcpy(&bmp[26], &planes_zero, 2);
    auto res = BmpTool::load(std::span<const uint8_t>(bmp.data(), bmp.size()));
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidImageHeader);

    // biPlanes = 2
    uint16_t planes_two = 2;
    std::memcpy(&bmp[26], &planes_two, 2);
    res = BmpTool::load(std::span<const uint8_t>(bmp.data(), bmp.size()));
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidImageHeader);
}

TEST(FuzzRegressionTest, CorruptCompressionCodes) {
    auto bmp = createValidBmp24bpp(2, 2);

    // BI_RLE8 (1)
    uint32_t rle8 = 1;
    std::memcpy(&bmp[30], &rle8, 4);
    auto res = BmpTool::load(std::span<const uint8_t>(bmp.data(), bmp.size()));
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error(), BmpTool::BitmapError::UnsupportedCompression);

    // BI_RLE4 (2)
    uint32_t rle4 = 2;
    std::memcpy(&bmp[30], &rle4, 4);
    res = BmpTool::load(std::span<const uint8_t>(bmp.data(), bmp.size()));
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error(), BmpTool::BitmapError::UnsupportedCompression);

    // BI_BITFIELDS (3)
    uint32_t bitfields = 3;
    std::memcpy(&bmp[30], &bitfields, 4);
    res = BmpTool::load(std::span<const uint8_t>(bmp.data(), bmp.size()));
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error(), BmpTool::BitmapError::UnsupportedCompression);
}

TEST(FuzzRegressionTest, CorruptBitDepths) {
    auto bmp = createValidBmp24bpp(2, 2);

    std::vector<uint16_t> invalid_bpps = {0, 1, 4, 8, 16, 48, 64};
    for (uint16_t bpp : invalid_bpps) {
        std::memcpy(&bmp[28], &bpp, 2);
        auto res = BmpTool::load(std::span<const uint8_t>(bmp.data(), bmp.size()));
        EXPECT_TRUE(res.isError());
        EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidColorDepth) << "Failed for bpp=" << bpp;
    }
}

TEST(FuzzRegressionTest, CorruptExtremeDimensions) {
    auto bmp = createValidBmp24bpp(2, 2);

    // Zero width
    int32_t zero_w = 0;
    std::memcpy(&bmp[18], &zero_w, 4);
    auto res = BmpTool::load(std::span<const uint8_t>(bmp.data(), bmp.size()));
    EXPECT_TRUE(res.isError());

    // Zero height
    bmp = createValidBmp24bpp(2, 2);
    int32_t zero_h = 0;
    std::memcpy(&bmp[22], &zero_h, 4);
    res = BmpTool::load(std::span<const uint8_t>(bmp.data(), bmp.size()));
    EXPECT_TRUE(res.isError());

    // Negative width
    bmp = createValidBmp24bpp(2, 2);
    int32_t neg_w = -2;
    std::memcpy(&bmp[18], &neg_w, 4);
    res = BmpTool::load(std::span<const uint8_t>(bmp.data(), bmp.size()));
    EXPECT_TRUE(res.isError());

    // Exceeds max dimension (70000 > 65536)
    bmp = createValidBmp24bpp(2, 2);
    int32_t huge_w = 70000;
    std::memcpy(&bmp[18], &huge_w, 4);
    res = BmpTool::load(std::span<const uint8_t>(bmp.data(), bmp.size()));
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error(), BmpTool::BitmapError::ExceedsMaxDimensions);

    // INT32_MIN height -> InvalidImageHeader
    bmp = createValidBmp24bpp(2, 2);
    int32_t min_h = std::numeric_limits<int32_t>::min();
    std::memcpy(&bmp[22], &min_h, 4);
    res = BmpTool::load(std::span<const uint8_t>(bmp.data(), bmp.size()));
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidImageHeader);
}

TEST(FuzzRegressionTest, SystematicByteByByteTruncation24bpp) {
    // Valid 3x3 24bpp (has row padding: 3*3=9 -> 12 bytes/row, total 54 + 36 = 90 bytes)
    auto full_bmp = createValidBmp24bpp(3, 3);
    ASSERT_EQ(full_bmp.size(), 90u);

    // Verify full BMP loads cleanly (canonical loaded output is 32bpp RGBA)
    auto full_res = BmpTool::load(std::span<const uint8_t>(full_bmp.data(), full_bmp.size()));
    ASSERT_TRUE(full_res.isSuccess());
    EXPECT_EQ(full_res.value().w, 3);
    EXPECT_EQ(full_res.value().h, 3);
    EXPECT_EQ(full_res.value().bpp, 32);

    // Systematically truncate at EVERY single byte offset from 0 up to 89
    for (size_t len = 0; len < full_bmp.size(); ++len) {
        auto res = BmpTool::load(std::span<const uint8_t>(full_bmp.data(), len));
        EXPECT_TRUE(res.isError()) << "Truncated buffer of length " << len << " unexpectedly succeeded!";

        if (len < 14) {
            EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidFileHeader) << "Length: " << len;
        } else if (len < 54) {
            EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidImageHeader) << "Length: " << len;
        } else if (len == 54) {
            // At len == 54, bfOffBits (54) >= bmp_data.size() (54), which triggers InvalidFileHeader
            EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidFileHeader) << "Length: " << len;
        } else {
            // Header is fully present, but payload is truncated
            EXPECT_EQ(res.error(), BmpTool::BitmapError::PayloadTruncated) << "Length: " << len;
        }
    }
}

TEST(FuzzRegressionTest, SystematicByteByByteTruncation32bpp) {
    // Valid 2x2 32bpp (total 54 + 16 = 70 bytes)
    auto full_bmp = createValidBmp32bpp(2, 2);
    ASSERT_EQ(full_bmp.size(), 70u);

    auto full_res = BmpTool::load(std::span<const uint8_t>(full_bmp.data(), full_bmp.size()));
    ASSERT_TRUE(full_res.isSuccess());
    EXPECT_EQ(full_res.value().w, 2);
    EXPECT_EQ(full_res.value().h, 2);
    EXPECT_EQ(full_res.value().bpp, 32);

    for (size_t len = 0; len < full_bmp.size(); ++len) {
        auto res = BmpTool::load(std::span<const uint8_t>(full_bmp.data(), len));
        EXPECT_TRUE(res.isError()) << "Truncated buffer of length " << len << " unexpectedly succeeded!";

        if (len < 14) {
            EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidFileHeader) << "Length: " << len;
        } else if (len < 54) {
            EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidImageHeader) << "Length: " << len;
        } else if (len == 54) {
            EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidFileHeader) << "Length: " << len;
        } else {
            EXPECT_EQ(res.error(), BmpTool::BitmapError::PayloadTruncated) << "Length: " << len;
        }
    }
}

TEST(FuzzRegressionTest, SystematicTopDownTruncation) {
    // Top-down 2x2 32bpp (negative height in header = -2, loaded as positive height = 2)
    auto full_bmp = createValidBmp32bpp(2, -2);
    ASSERT_EQ(full_bmp.size(), 70u);

    auto full_res = BmpTool::load(std::span<const uint8_t>(full_bmp.data(), full_bmp.size()));
    ASSERT_TRUE(full_res.isSuccess());
    EXPECT_EQ(full_res.value().w, 2);
    EXPECT_EQ(full_res.value().h, 2);

    for (size_t len = 0; len < full_bmp.size(); ++len) {
        auto res = BmpTool::load(std::span<const uint8_t>(full_bmp.data(), len));
        EXPECT_TRUE(res.isError()) << "Truncated top-down buffer of length " << len << " unexpectedly succeeded!";

        if (len < 14) {
            EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidFileHeader);
        } else if (len < 54) {
            EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidImageHeader);
        } else if (len == 54) {
            EXPECT_EQ(res.error(), BmpTool::BitmapError::InvalidFileHeader);
        } else {
            EXPECT_EQ(res.error(), BmpTool::BitmapError::PayloadTruncated);
        }
    }
}
