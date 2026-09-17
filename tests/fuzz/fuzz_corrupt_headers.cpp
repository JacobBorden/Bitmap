#include "../../include/bitmap.hpp"
#include <cstdint>
#include <vector>
#include <span>
#include <cstring>
#include <algorithm>

// Fuzz target designed specifically to test BITMAPFILEHEADER (14 bytes) and
// BITMAPINFOHEADER (40 bytes) permutations, corrupted fields, and boundary values.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    if (Size == 0) {
        return 0;
    }

    // Baseline template of a valid 1x1 24-bpp BMP (54 header bytes + 4 payload bytes = 58 bytes)
    // bfType='BM'(0x4D42), bfSize=58, bfReserved1=0, bfReserved2=0, bfOffBits=54
    // biSize=40, biWidth=1, biHeight=1, biPlanes=1, biBitCount=24, biCompression=0,
    // biSizeImage=4, biXPelsPerMeter=0, biYPelsPerMeter=0, biClrUsed=0, biClrImportant=0
    static const uint8_t base_valid_bmp[58] = {
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
        0x00, 0x00, 0x00, 0x00, // biCompression = 0 (BI_RGB)
        0x04, 0x00, 0x00, 0x00, // biSizeImage = 4 (1 pixel 24bpp padded to 4 bytes)
        0x00, 0x00, 0x00, 0x00, // biXPelsPerMeter
        0x00, 0x00, 0x00, 0x00, // biYPelsPerMeter
        0x00, 0x00, 0x00, 0x00, // biClrUsed
        0x00, 0x00, 0x00, 0x00, // biClrImportant
        0xFF, 0x00, 0x80, 0x00  // 3 bytes BGR + 1 byte padding
    };

    // Mode 1: Direct buffer test with mutated headers overlaid on base template
    {
        std::vector<uint8_t> buffer(base_valid_bmp, base_valid_bmp + sizeof(base_valid_bmp));
        size_t header_mutate_len = std::min(Size, static_cast<size_t>(54));
        std::memcpy(buffer.data(), Data, header_mutate_len);

        // If extra bytes provided, append as variable payload up to 16KB
        if (Size > 54) {
            size_t payload_len = std::min(Size - 54, static_cast<size_t>(16384));
            buffer.insert(buffer.end(), Data + 54, Data + 54 + payload_len);
        }

        [[maybe_unused]] auto result = BmpTool::load(std::span<const uint8_t>(buffer.data(), buffer.size()));
    }

    // Mode 2: Field-guided mutations using libFuzzer structured input
    if (Size >= 16) {
        std::vector<uint8_t> buffer(base_valid_bmp, base_valid_bmp + sizeof(base_valid_bmp));
        
        // Pick individual fields to mutate based on fuzzer data
        uint8_t field_selector = Data[0];
        
        // bfOffBits mutation (offset 10..13)
        if (field_selector & 0x01) {
            std::memcpy(&buffer[10], &Data[1], std::min(sizeof(uint32_t), Size - 1));
        }
        // biWidth mutation (offset 18..21)
        if (field_selector & 0x02 && Size >= 5) {
            std::memcpy(&buffer[18], &Data[2], std::min(sizeof(uint32_t), Size - 2));
        }
        // biHeight mutation (offset 22..25)
        if (field_selector & 0x04 && Size >= 6) {
            std::memcpy(&buffer[22], &Data[3], std::min(sizeof(uint32_t), Size - 3));
        }
        // biBitCount mutation (offset 28..29)
        if (field_selector & 0x08 && Size >= 7) {
            std::memcpy(&buffer[28], &Data[4], std::min(sizeof(uint16_t), Size - 4));
        }
        // biCompression mutation (offset 30..33)
        if (field_selector & 0x10 && Size >= 8) {
            std::memcpy(&buffer[30], &Data[5], std::min(sizeof(uint32_t), Size - 5));
        }
        // biSizeImage mutation (offset 34..37)
        if (field_selector & 0x20 && Size >= 9) {
            std::memcpy(&buffer[34], &Data[6], std::min(sizeof(uint32_t), Size - 6));
        }
        // biSize mutation (offset 14..17)
        if (field_selector & 0x40 && Size >= 10) {
            std::memcpy(&buffer[14], &Data[7], std::min(sizeof(uint32_t), Size - 7));
        }

        [[maybe_unused]] auto result = BmpTool::load(std::span<const uint8_t>(buffer.data(), buffer.size()));
    }

    return 0;
}
