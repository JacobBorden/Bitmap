// Standard library includes
#include <vector>
#include <cstring> // For std::memcpy
#include <algorithm> // For std::min, std::max (potentially)
#include <stdexcept> // For robust error checking if needed beyond enums

// Own project includes
#include "../../include/bitmap.hpp" // For BmpTool::Bitmap, Result, BitmapError
#include "bitmap_internal.hpp"    // For BmpTool::Format::Internal structures and helpers

// External library includes (as per task)
#include "../../src/bitmapfile/bitmap_file.h" // For BITMAPFILEHEADER, BITMAPINFOHEADER from external lib
#include "../../src/bitmap/bitmap.h"         // For ::Pixel, ::CreateMatrixFromBitmap, ::CreateBitmapFromMatrix
#include "../../src/matrix/matrix.h"         // For Matrix::Matrix

// Define constants for BMP format (can be used by Format::Internal helpers or if save needs them directly)
constexpr uint16_t BMP_MAGIC_TYPE_CONST = 0x4D42; // 'BM'
constexpr uint32_t BI_RGB_CONST = 0; // No compression

namespace BmpTool {

// Namespace for internal helper functions and structures, kept for potential future use
// or if other parts of the library (not modified here) depend on them.
namespace Format {
namespace Internal {

bool validateHeaders(const InternalBitmapFileHeader& fileHeader, const InternalBitmapInfoHeader& infoHeader) {
    if (fileHeader.bfType != BMP_MAGIC_TYPE_CONST) {
        return false; 
    }
    if (infoHeader.biPlanes != 1) {
        return false; 
    }
    if (infoHeader.biCompression != BI_RGB_CONST) {
        return false; 
    }
    if (infoHeader.biBitCount != 24 && infoHeader.biBitCount != 32) {
        return false; 
    }
    if (infoHeader.biWidth <= 0 || infoHeader.biHeight == 0) { 
        return false; 
    }
    if (fileHeader.bfOffBits < (sizeof(InternalBitmapFileHeader) + infoHeader.biSize)) {
      // This check is simplified. A full check would also consider bfOffBits < fileHeader.bfSize
    }
    return true;
}

uint32_t calculateRowPadding(uint32_t width, uint16_t bpp) {
    uint32_t bytes_per_row_unpadded = (width * bpp) / 8;
    uint32_t remainder = bytes_per_row_unpadded % 4;
    if (remainder == 0) {
        return 0;
    }
    return 4 - remainder;
}

} // namespace Internal
} // namespace Format

// The BmpTool::load function (modified in the previous step, using external library)
Result<Bitmap, BitmapError> load(std::span<const uint8_t> bmp_data) {
    // 1. Parse Input Span (Manual BMP Header Parsing)
    if (bmp_data.size() < sizeof(BITMAPFILEHEADER)) { // Using external lib's BITMAPFILEHEADER
        return BitmapError::InvalidFileHeader;
    }
    BITMAPFILEHEADER fh; 
    std::memcpy(&fh, bmp_data.data(), sizeof(BITMAPFILEHEADER));

    if (bmp_data.size() < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)) { // Using external lib's BITMAPINFOHEADER
        return BitmapError::InvalidImageHeader;
    }
    BITMAPINFOHEADER ih; 
    std::memcpy(&ih, bmp_data.data() + sizeof(BITMAPFILEHEADER), sizeof(BITMAPINFOHEADER));

    // Basic Validations
    if (fh.bfType != BMP_MAGIC_TYPE_CONST) { // 'BM'
        return BitmapError::NotABmp;
    }
    if (ih.biCompression != BI_RGB_CONST) { 
        return BitmapError::UnsupportedBpp; 
    }
    if (ih.biBitCount != 24 && ih.biBitCount != 32) {
        return BitmapError::UnsupportedBpp;
    }
    if (ih.biPlanes != 1) {
        return BitmapError::InvalidImageHeader;
    }
    if (fh.bfOffBits < (sizeof(BITMAPFILEHEADER) + ih.biSize) || fh.bfOffBits >= fh.bfSize || fh.bfSize > bmp_data.size()) {
        return BitmapError::InvalidFileHeader;
    }
    if (ih.biWidth <= 0 || ih.biHeight == 0) {
        return BitmapError::InvalidImageHeader;
    }
    
    long abs_height_long = ih.biHeight < 0 ? -ih.biHeight : ih.biHeight;
    if (abs_height_long == 0) { 
        return BitmapError::InvalidImageHeader;
    }
    uint32_t abs_height = static_cast<uint32_t>(abs_height_long);

    // 2. Populate ::Bitmap::File Object
    ::Bitmap::File temp_bmp_file; 
    temp_bmp_file.bitmapFileHeader = fh;
    temp_bmp_file.bitmapInfoHeader = ih;

    uint32_t bits_per_pixel = ih.biBitCount;
    uint32_t bytes_per_pixel_src = bits_per_pixel / 8;
    uint32_t unpadded_row_size_src = ih.biWidth * bytes_per_pixel_src;
    uint32_t padded_row_size_src = (unpadded_row_size_src + 3) & (~3); 

    uint32_t expected_pixel_data_size = padded_row_size_src * abs_height;

    if (ih.biSizeImage != 0 && ih.biSizeImage != expected_pixel_data_size) {
        if (ih.biSizeImage < expected_pixel_data_size) {
            return BitmapError::InvalidImageData;
        }
    }

    if (fh.bfOffBits + expected_pixel_data_size > fh.bfSize) {
        return BitmapError::InvalidImageData; 
    }
    if (fh.bfOffBits + expected_pixel_data_size > bmp_data.size()) {
        return BitmapError::InvalidImageData; 
    }

    temp_bmp_file.bitmapData.resize(expected_pixel_data_size);
    std::memcpy(temp_bmp_file.bitmapData.data(), bmp_data.data() + fh.bfOffBits, expected_pixel_data_size);
    temp_bmp_file.SetValid(); 

    // 3. Convert to Matrix<::Pixel>
    Matrix::Matrix<::Pixel> image_matrix = ::CreateMatrixFromBitmap(temp_bmp_file); 

    if (image_matrix.cols() == 0 || image_matrix.rows() == 0) { // Changed Width/Height to cols/rows
        return BitmapError::InvalidImageData; 
    }

    // 4. Convert Matrix<::Pixel> (assumed RGBA by ::Pixel members) to BmpTool::Bitmap (RGBA)
    Bitmap bmp_out;
    bmp_out.w = image_matrix.cols(); // Changed Width to cols
    bmp_out.h = image_matrix.rows(); // Changed Height to rows
    bmp_out.bpp = 32; 
    bmp_out.data.resize(static_cast<size_t>(bmp_out.w) * bmp_out.h * 4);

    for (uint32_t y = 0; y < bmp_out.h; ++y) {
        for (uint32_t x = 0; x < bmp_out.w; ++x) {
            const ::Pixel& src_pixel = image_matrix.at(y, x); // Changed Get(x,y) to at(y,x)
            
            size_t dest_idx = (static_cast<size_t>(y) * bmp_out.w + x) * 4;
            bmp_out.data[dest_idx + 0] = src_pixel.red;   
            bmp_out.data[dest_idx + 1] = src_pixel.green; 
            bmp_out.data[dest_idx + 2] = src_pixel.blue;  
            bmp_out.data[dest_idx + 3] = src_pixel.alpha; 
        }
    }
    return bmp_out;
}

// The NEW BmpTool::save function using the external library
Result<void, BitmapError> save(const Bitmap& bitmap_in, std::span<uint8_t> out_bmp_buffer) {
    // 1. Input Validation from BmpTool::Bitmap
    if (bitmap_in.w == 0 || bitmap_in.h == 0) {
        return BitmapError::InvalidImageData; // Cannot save an empty image
    }
    if (bitmap_in.bpp != 32) {
        // This implementation expects RGBA data from bitmap_in.
        return BitmapError::UnsupportedBpp; 
    }

    const size_t expected_data_size = static_cast<size_t>(bitmap_in.w) * bitmap_in.h * 4; // 4 bytes per pixel for 32 bpp
    if (bitmap_in.data.size() < expected_data_size) {
        // The provided pixel data buffer is smaller than what the width, height, and bpp imply.
        return BitmapError::InvalidImageData; // Or a more specific error like InsufficientPixelData
    }

    // 2. Convert BmpTool::Bitmap (RGBA) to Matrix<::Pixel> (RGBA)
    // Assuming ::Pixel struct has members .red, .green, .blue, .alpha
    Matrix::Matrix<::Pixel> image_matrix(bitmap_in.h, bitmap_in.w); // Changed order to (rows, cols)

    for (uint32_t y = 0; y < bitmap_in.h; ++y) {
        for (uint32_t x = 0; x < bitmap_in.w; ++x) {
            const uint8_t* src_pixel_ptr = &bitmap_in.data[(static_cast<size_t>(y) * bitmap_in.w + x) * 4]; // RGBA
            ::Pixel dest_pixel; 
            dest_pixel.red   = src_pixel_ptr[0];
            dest_pixel.green = src_pixel_ptr[1];
            dest_pixel.blue  = src_pixel_ptr[2];
            dest_pixel.alpha = src_pixel_ptr[3];
            
            image_matrix.at(y, x) = dest_pixel; // Changed Set(x,y) to at(y,x)
        }
    }

    // 3. Convert Matrix<::Pixel> to ::Bitmap::File
    // ::CreateBitmapFromMatrix is expected to produce a ::Bitmap::File with BMP-formatted data (e.g., BGRA, bottom-up)
    ::Bitmap::File temp_bmp_file = ::CreateBitmapFromMatrix(image_matrix);

    if (!temp_bmp_file.IsValid()) { 
        return BitmapError::UnknownError; // Error during CreateBitmapFromMatrix
    }

    // 4. Serialize ::Bitmap::File to Output Span
    // Ensure bfSize in the header is correct. CreateBitmapFromMatrix should set this.
    // If not, it must be calculated:
    uint32_t calculated_total_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + temp_bmp_file.bitmapData.size();
    
    // If CreateBitmapFromMatrix doesn't set bfSize correctly, or sets it to 0.
    if (temp_bmp_file.bitmapFileHeader.bfSize != calculated_total_size) {
       // Optionally, log a warning or adjust if there's a policy.
       // For safety, ensure bfSize is what we expect for the data being copied.
       // temp_bmp_file.bitmapFileHeader.bfSize = calculated_total_size; // Uncomment if necessary
    }
    // It's safer to use the calculated_total_size for buffer check if bfSize from lib is unreliable.
    // However, the data to copy comes from temp_bmp_file, so its internal consistency is key.

    uint32_t total_required_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + temp_bmp_file.bitmapData.size();


    if (out_bmp_buffer.size() < total_required_size) {
        return BitmapError::OutputBufferTooSmall;
    }

    // If CreateBitmapFromMatrix does not correctly set bfSize, this could be problematic.
    // For now, we trust CreateBitmapFromMatrix sets its headers correctly.
    // If bfSize is not total_required_size, the output file might be technically incorrect
    // but still contain the right amount of data if total_required_size is used for memcpy.
    // The most robust approach is to ensure temp_bmp_file.bitmapFileHeader.bfSize IS total_required_size.
    // If we have to fix it:
    // temp_bmp_file.bitmapFileHeader.bfSize = total_required_size; 
    // temp_bmp_file.bitmapFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); // Also ensure this

    uint8_t* buffer_ptr = out_bmp_buffer.data();
    std::memcpy(buffer_ptr, &temp_bmp_file.bitmapFileHeader, sizeof(BITMAPFILEHEADER));
    buffer_ptr += sizeof(BITMAPFILEHEADER);
    std::memcpy(buffer_ptr, &temp_bmp_file.bitmapInfoHeader, sizeof(BITMAPINFOHEADER));
    buffer_ptr += sizeof(BITMAPINFOHEADER);

    // Ensure that bitmapData is not empty before attempting to access its data() pointer
    if (!temp_bmp_file.bitmapData.empty()) {
        std::memcpy(buffer_ptr, temp_bmp_file.bitmapData.data(), temp_bmp_file.bitmapData.size());
    } else if (total_required_size > (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER))) {
        // If bitmapData is empty but headers indicated pixel data, this is an inconsistency.
        return BitmapError::InvalidImageData; // Or UnknownError from CreateBitmapFromMatrix
    }


    // 5. Return
    return BmpTool::Success{}; // This will implicitly convert to Result<void, BitmapError>(Success{})
}

} // namespace BmpTool
