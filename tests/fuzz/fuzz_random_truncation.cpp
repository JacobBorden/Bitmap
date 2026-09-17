#include "../../include/bitmap.hpp"
#include <cstdint>
#include <vector>
#include <span>
#include <cstring>
#include <algorithm>

// Valid 1x1 24-bpp BMP (58 bytes total)
static const uint8_t valid_1x1_24bpp[58] = {
    0x42, 0x4D,             // bfType = 'BM'
    0x3A, 0x00, 0x00, 0x00, // bfSize = 58
    0x00, 0x00,             // bfReserved1 = 0
    0x00, 0x00,             // bfReserved2 = 0
    0x36, 0x00, 0x00, 0x00, // bfOffBits = 54
    0x28, 0x00, 0x00, 0x00, // biSize = 40
    0x01, 0x00, 0x00, 0x00, // biWidth = 1
    0x01, 0x00, 0x00, 0x00, // biHeight = 1
    0x01, 0x00,             // biPlanes = 1
    0x18, 0x00,             // biBitCount = 24
    0x00, 0x00, 0x00, 0x00, // biCompression = 0
    0x04, 0x00, 0x00, 0x00, // biSizeImage = 4
    0x00, 0x00, 0x00, 0x00, // biXPelsPerMeter
    0x00, 0x00, 0x00, 0x00, // biYPelsPerMeter
    0x00, 0x00, 0x00, 0x00, // biClrUsed
    0x00, 0x00, 0x00, 0x00, // biClrImportant
    0x11, 0x22, 0x33, 0x00  // 3 bytes pixel + 1 byte padding
};

// Valid 2x2 32-bpp BMP (70 bytes total: 54 header + 16 payload)
static const uint8_t valid_2x2_32bpp[70] = {
    0x42, 0x4D,             // bfType = 'BM'
    0x46, 0x00, 0x00, 0x00, // bfSize = 70
    0x00, 0x00,             // bfReserved1 = 0
    0x00, 0x00,             // bfReserved2 = 0
    0x36, 0x00, 0x00, 0x00, // bfOffBits = 54
    0x28, 0x00, 0x00, 0x00, // biSize = 40
    0x02, 0x00, 0x00, 0x00, // biWidth = 2
    0x02, 0x00, 0x00, 0x00, // biHeight = 2
    0x01, 0x00,             // biPlanes = 1
    0x20, 0x00,             // biBitCount = 32
    0x00, 0x00, 0x00, 0x00, // biCompression = 0
    0x10, 0x00, 0x00, 0x00, // biSizeImage = 16
    0x00, 0x00, 0x00, 0x00, // biXPelsPerMeter
    0x00, 0x00, 0x00, 0x00, // biYPelsPerMeter
    0x00, 0x00, 0x00, 0x00, // biClrUsed
    0x00, 0x00, 0x00, 0x00, // biClrImportant
    // 4 pixels x 4 bytes = 16 bytes
    0x10, 0x20, 0x30, 0xFF,  0x40, 0x50, 0x60, 0xFF,
    0x70, 0x80, 0x90, 0xFF,  0xA0, 0xB0, 0xC0, 0xFF
};

// Valid top-down (-2 height) 2x2 32-bpp BMP
static const uint8_t valid_topdown_2x2_32bpp[70] = {
    0x42, 0x4D,             // bfType = 'BM'
    0x46, 0x00, 0x00, 0x00, // bfSize = 70
    0x00, 0x00,             // bfReserved1 = 0
    0x00, 0x00,             // bfReserved2 = 0
    0x36, 0x00, 0x00, 0x00, // bfOffBits = 54
    0x28, 0x00, 0x00, 0x00, // biSize = 40
    0x02, 0x00, 0x00, 0x00, // biWidth = 2
    0xFE, 0xFF, 0xFF, 0xFF, // biHeight = -2 (top-down)
    0x01, 0x00,             // biPlanes = 1
    0x20, 0x00,             // biBitCount = 32
    0x00, 0x00, 0x00, 0x00, // biCompression = 0
    0x10, 0x00, 0x00, 0x00, // biSizeImage = 16
    0x00, 0x00, 0x00, 0x00, // biXPelsPerMeter
    0x00, 0x00, 0x00, 0x00, // biYPelsPerMeter
    0x00, 0x00, 0x00, 0x00, // biClrUsed
    0x00, 0x00, 0x00, 0x00, // biClrImportant
    0x10, 0x20, 0x30, 0xFF,  0x40, 0x50, 0x60, 0xFF,
    0x70, 0x80, 0x90, 0xFF,  0xA0, 0xB0, 0xC0, 0xFF
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    if (Size == 0) {
        return 0;
    }

    // 1. Truncate valid templates based on input byte offsets
    // Test 1x1 24bpp at byte offset derived from Data
    {
        size_t cut_1 = Data[0] % (sizeof(valid_1x1_24bpp) + 1);
        auto res = BmpTool::load(std::span<const uint8_t>(valid_1x1_24bpp, cut_1));
        if (cut_1 < sizeof(valid_1x1_24bpp)) {
            // Must not succeed if truncated
            if (res.isSuccess()) {
                return -1;
            }
        }
    }

    // Test 2x2 32bpp at byte offset derived from Data
    if (Size >= 2) {
        size_t cut_2 = Data[1] % (sizeof(valid_2x2_32bpp) + 1);
        auto res = BmpTool::load(std::span<const uint8_t>(valid_2x2_32bpp, cut_2));
        if (cut_2 < sizeof(valid_2x2_32bpp)) {
            if (res.isSuccess()) {
                return -1;
            }
        }
    }

    // Test top-down 2x2 at byte offset derived from Data
    if (Size >= 3) {
        size_t cut_3 = Data[2] % (sizeof(valid_topdown_2x2_32bpp) + 1);
        auto res = BmpTool::load(std::span<const uint8_t>(valid_topdown_2x2_32bpp, cut_3));
        if (cut_3 < sizeof(valid_topdown_2x2_32bpp)) {
            if (res.isSuccess()) {
                return -1;
            }
        }
    }

    // 2. Fuzz-provided buffer truncated at every offset up to 64 bytes
    size_t max_scan = std::min(Size, static_cast<size_t>(64));
    for (size_t cut = 0; cut <= max_scan; ++cut) {
        [[maybe_unused]] auto res = BmpTool::load(std::span<const uint8_t>(Data, cut));
    }

    // 3. Fuzz-provided buffer truncated at random offset derived from Data
    size_t arbitrary_cut = (Size > 0) ? (Data[Size - 1] % (Size + 1)) : 0;
    [[maybe_unused]] auto res = BmpTool::load(std::span<const uint8_t>(Data, arbitrary_cut));

    return 0;
}
