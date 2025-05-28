#pragma once

#include <cstdint> // For fixed-width integer types like uint16_t, uint32_t

namespace BmpTool {
namespace Format {
namespace Internal {

// Ensure structures are packed to match file format
#pragma pack(push, 1)

/**
 * @brief Internal representation of the BMP file header.
 * Corresponds to the BITMAPFILEHEADER structure in the BMP file format.
 */
struct InternalBitmapFileHeader {
    uint16_t bfType;      ///< Specifies the file type, must be 0x4D42 ('BM').
    uint32_t bfSize;      ///< Specifies the size, in bytes, of the bitmap file.
    uint16_t bfReserved1; ///< Reserved; must be zero.
    uint16_t bfReserved2; ///< Reserved; must be zero.
    uint32_t bfOffBits;   ///< Specifies the offset, in bytes, from the beginning of the
                          ///< InternalBitmapFileHeader structure to the bitmap bits.
};

/**
 * @brief Internal representation of the BMP information header.
 * Corresponds to the BITMAPINFOHEADER structure in the BMP file format.
 */
struct InternalBitmapInfoHeader {
    uint32_t biSize;          ///< Specifies the number of bytes required by the structure.
    int32_t  biWidth;         ///< Specifies the width of the bitmap, in pixels.
    int32_t  biHeight;        ///< Specifies the height of the bitmap, in pixels.
                              ///< If biHeight is positive, the bitmap is a bottom-up DIB.
                              ///< If biHeight is negative, the bitmap is a top-down DIB.
    uint16_t biPlanes;        ///< Specifies the number of planes for the target device. Must be 1.
    uint16_t biBitCount;      ///< Specifies the number of bits-per-pixel (bpp).
                              ///< Common values are 1, 4, 8, 16, 24, and 32.
    uint32_t biCompression;   ///< Specifies the type of compression for a compressed bottom-up bitmap.
                              ///< 0 (BI_RGB) means uncompressed.
    uint32_t biSizeImage;     ///< Specifies the size, in bytes, of the image.
                              ///< This may be set to zero for BI_RGB bitmaps.
    int32_t  biXPelsPerMeter; ///< Specifies the horizontal resolution, in pixels-per-meter, of the target device.
    int32_t  biYPelsPerMeter; ///< Specifies the vertical resolution, in pixels-per-meter, of the target device.
    uint32_t biClrUsed;       ///< Specifies the number of color indexes in the color table that are actually used by the bitmap.
    uint32_t biClrImportant;  ///< Specifies the number of color indexes that are required for displaying the bitmap.
                              ///< If this value is zero, all colors are required.
};

#pragma pack(pop)

/**
 * @brief Validates the essential fields of the BMP file and info headers.
 *
 * Checks for correct magic number, supported bits-per-pixel, and other common constraints.
 *
 * @param fileHeader The internal file header structure to validate.
 * @param infoHeader The internal info header structure to validate.
 * @return True if the headers are considered valid for basic processing, false otherwise.
 */
bool validateHeaders(const InternalBitmapFileHeader& fileHeader, const InternalBitmapInfoHeader& infoHeader);

/**
 * @brief Calculates the number of padding bytes needed for each row in a BMP image.
 *
 * BMP image rows are padded to be a multiple of 4 bytes.
 *
 * @param width The width of the bitmap in pixels.
 * @param bpp The bits per pixel of the bitmap.
 * @return The number of padding bytes (0 to 3) required for each row.
 */
uint32_t calculateRowPadding(uint32_t width, uint16_t bpp);

} // namespace Internal
} // namespace Format
} // namespace BmpTool
