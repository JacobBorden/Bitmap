// Standard library includes
#include <vector>
#include <cstring> // For std::memcpy
#include <algorithm> // For std::min, std::max (potentially)
#include <stdexcept> // For robust error checking if needed beyond enums
#include <limits>    // Required for std::numeric_limits

// Own project includes
#include "../../include/bitmap.hpp" // For BmpTool::Bitmap, Result, BitmapError

// External library includes (as per task)
#include "../../src/bitmapfile/bitmap_file.h" // For BITMAPFILEHEADER, BITMAPINFOHEADER from external lib
#include "../../src/bitmap/bitmap.h"         // For ::Pixel, ::CreateMatrixFromBitmap, ::CreateBitmapFromMatrix
#include "../../src/matrix/matrix.h"         // For Matrix::Matrix
#include "../simd_utils.hpp" // Added include

// Define constants for BMP format (can be used by Format::Internal helpers or if save needs them directly)
// These constants are still used by the load function.
constexpr uint16_t BMP_MAGIC_TYPE_CONST = 0x4D42; // 'BM'
constexpr uint32_t BI_RGB_CONST = 0; // No compression

namespace BmpTool {

// Forward declarations for internal helper functions, previously in format_internal_helpers.hpp
void internal_swizzle_bgra_to_rgba_simd(const ::Pixel* src_bgra_pixels, uint8_t* dest_rgba_data, size_t num_pixels);
void internal_swizzle_rgba_to_bgra_simd(const uint8_t* src_rgba_data, ::Pixel* dest_bgra_pixels, size_t num_pixels);

// The BmpTool::Format::Internal namespace and its functions are removed as they are no longer used.

Result<Bitmap, BitmapError> load(std::span<const uint8_t> bmp_data) {
    // 1. Read BITMAPFILEHEADER and BITMAPINFOHEADER
    if (bmp_data.size() < sizeof(BITMAPFILEHEADER)) {
        return BitmapError::InvalidFileHeader;
    }
    BITMAPFILEHEADER fh;
    std::memcpy(&fh, bmp_data.data(), sizeof(BITMAPFILEHEADER));

    if (bmp_data.size() < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)) {
        return BitmapError::InvalidImageHeader;
    }
    BITMAPINFOHEADER ih;
    std::memcpy(&ih, bmp_data.data() + sizeof(BITMAPFILEHEADER), sizeof(BITMAPINFOHEADER));

    // 2. Perform essential early checks
    if (fh.bfType != BMP_MAGIC_TYPE_CONST) {
        return BitmapError::NotABmp;
    }
    if (ih.biCompression != BI_RGB_CONST) {
        // CreateMatrixFromBitmap doesn't explicitly check this, but assumes uncompressed.
        return BitmapError::UnsupportedBpp;
    }
    if (ih.biBitCount != 24 && ih.biBitCount != 32) {
        // CreateMatrixFromBitmap checks this.
        return BitmapError::UnsupportedBpp;
    }
    if (ih.biWidth <= 0 || ih.biHeight == 0) { // abs(ih.biHeight) > 0 is covered by ih.biHeight == 0
        // CreateMatrixFromBitmap checks this via matrix dimensions.
        return BitmapError::InvalidImageHeader;
    }
    if (fh.bfOffBits < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) || fh.bfOffBits >= bmp_data.size()) {
        return BitmapError::InvalidFileHeader;
    }
    // Check for bfOffBits + ih.biSizeImage <= bmp_data.size() if ih.biSizeImage is not zero.
    // This check is more reliably done after calculating expected_pixel_data_size.
    // if (ih.biSizeImage != 0 && (fh.bfOffBits + ih.biSizeImage > bmp_data.size())) {
    //     return BitmapError::InvalidImageData;
    // }


    // 3. Create a ::Bitmap::File object
    ::Bitmap::File temp_bmp_file;

    // 4. Assign fh and ih
    temp_bmp_file.bitmapFileHeader = fh;
    temp_bmp_file.bitmapInfoHeader = ih;

    // 5. Calculate the expected pixel data size
    uint32_t abs_height = (ih.biHeight < 0) ? -static_cast<uint32_t>(ih.biHeight) : static_cast<uint32_t>(ih.biHeight);
    if (abs_height == 0) { // Should have been caught by ih.biHeight == 0, but defensive check.
        return BitmapError::InvalidImageHeader;
    }
    uint32_t bytes_per_pixel_src = ih.biBitCount / 8;
    uint32_t unpadded_row_size_src = ih.biWidth * bytes_per_pixel_src;

    // Use Format::Internal::calculateRowPadding or replicate logic if we decide to remove the namespace entirely later
    // For now, let's assume calculateRowPadding is available or we can inline its logic if needed.
    // uint32_t padding_per_row = Format::Internal::calculateRowPadding(ih.biWidth, ih.biBitCount);
    // uint32_t padded_row_size_src = unpadded_row_size_src + padding_per_row;
    // More direct calculation for padded_row_size_src:
    uint32_t padded_row_size_src = (unpadded_row_size_src + 3) & (~3);

    // Prevent overflow for expected_pixel_data_size calculation
    if (abs_height > 0 && padded_row_size_src > (std::numeric_limits<uint32_t>::max() / abs_height) ) {
        return BitmapError::InvalidImageData; // Calculation would overflow
    }
    uint32_t expected_pixel_data_size = padded_row_size_src * abs_height;

    // 6. Check if fh.bfOffBits + expected_pixel_data_size <= bmp_data.size()
    if (fh.bfOffBits + expected_pixel_data_size > bmp_data.size()) {
        // This also covers the case where ih.biSizeImage might be 0 or incorrect,
        // relying on calculated size.
        return BitmapError::InvalidImageData;
    }

    // Additional check for ih.biSizeImage if it's provided and seems too small (though CreateMatrixFromBitmap might handle variations)
    // If ih.biSizeImage is present and smaller than calculated, it could be an issue.
    // However, the primary check is against bmp_data.size().
    if (ih.biSizeImage != 0 && ih.biSizeImage < expected_pixel_data_size) {
        // This might indicate a truncated BMP, even if bmp_data has enough bytes for expected_pixel_data_size
        // For now, we prioritize expected_pixel_data_size for buffer allocation.
        // CreateMatrixFromBitmap will be the final arbiter of data integrity.
    }


    // 7. Resize temp_bmp_file.bitmapData and copy the pixel data
    temp_bmp_file.bitmapData.resize(expected_pixel_data_size);
    if (expected_pixel_data_size > 0) { // Only copy if there's data to copy
      std::memcpy(temp_bmp_file.bitmapData.data(), bmp_data.data() + fh.bfOffBits, expected_pixel_data_size);
    }


    // 8. Call temp_bmp_file.SetValid()
    temp_bmp_file.SetValid(); // Assuming this marks the file as ready for CreateMatrixFromBitmap

    // 9. Call CreateMatrixFromBitmap
    Matrix::Matrix<::Pixel> image_matrix = ::CreateMatrixFromBitmap(temp_bmp_file);

    // 10. If image_matrix.cols() == 0 || image_matrix.rows() == 0, return error
    if (image_matrix.cols() == 0 || image_matrix.rows() == 0) {
        return BitmapError::InvalidImageData; // CreateMatrixFromBitmap failed to produce a valid matrix
    }

    // 11. Convert image_matrix to BmpTool::Bitmap bmp_out
    Bitmap bmp_out;
    bmp_out.w = image_matrix.cols();
    bmp_out.h = image_matrix.rows();
    bmp_out.bpp = 32; // Output is always 32bpp RGBA

    // Safeguard against overflow for bmp_out.data.resize
    if (bmp_out.h > 0 && bmp_out.w > (std::numeric_limits<size_t>::max() / bmp_out.h / 4)) { // 4 bytes per pixel
         return BitmapError::InvalidImageData; // Output image dimensions too large
    }
    bmp_out.data.resize(static_cast<size_t>(bmp_out.w) * bmp_out.h * 4);

    for (uint32_t y = 0; y < bmp_out.h; ++y) {
        const ::Pixel* src_bgra_pixels_row = &image_matrix[y][0]; // ::Pixel is BGRA
        uint8_t* dest_rgba_data_row = bmp_out.data.data() + (static_cast<size_t>(y) * bmp_out.w * 4);
        // Swizzle BGRA from ::Pixel to RGBA for BmpTool::Bitmap
        internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, bmp_out.w);
    }

    // 12. Return bmp_out
    return bmp_out;
}

// Helper function to convert an array of ::Pixel (BGRA order) to an array of uint8_t (RGBA order)
void internal_swizzle_bgra_to_rgba_simd(const ::Pixel* src_bgra_pixels, uint8_t* dest_rgba_data, size_t num_pixels) {
    size_t current_pixel_idx = 0;
    size_t num_pixels_to_process = num_pixels;

#if defined(__AVX2__)
    const size_t pixels_per_step = 8; // 8 pixels = 32 bytes
    __m256i shuffle_mask_bgra_to_rgba = _mm256_setr_epi8(
        2, 1, 0, 3,  6, 5, 4, 7,  10, 9, 8, 11,  14,13,12,15, // First 4 pixels (16 bytes)
        18,17,16,19, 22,21,20,23, 26,25,24,27, 30,29,28,31  // Next 4 pixels (16 bytes)
    );
    while (num_pixels_to_process >= pixels_per_step) {
        __m256i bgra_pixels_loaded = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src_bgra_pixels + current_pixel_idx));
        __m256i rgba_pixels = _mm256_shuffle_epi8(bgra_pixels_loaded, shuffle_mask_bgra_to_rgba);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(dest_rgba_data + current_pixel_idx * 4), rgba_pixels);
        current_pixel_idx += pixels_per_step;
        num_pixels_to_process -= pixels_per_step;
    }
#elif defined(__SSSE3__) // _mm_shuffle_epi8 requires SSSE3
    const size_t pixels_per_step = 4; // 4 pixels = 16 bytes
    __m128i shuffle_mask_bgra_to_rgba = _mm_setr_epi8(
        2, 1, 0, 3,  6, 5, 4, 7,  10, 9, 8, 11,  14,13,12,15
    );
    while (num_pixels_to_process >= pixels_per_step) {
        __m128i bgra_pixels_loaded = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src_bgra_pixels + current_pixel_idx));
        __m128i rgba_pixels = _mm_shuffle_epi8(bgra_pixels_loaded, shuffle_mask_bgra_to_rgba);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dest_rgba_data + current_pixel_idx * 4), rgba_pixels);
        current_pixel_idx += pixels_per_step;
        num_pixels_to_process -= pixels_per_step;
    }
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
    const size_t pixels_per_step = 4; // 4 pixels = 16 bytes using uint8x16_t
    const uint8_t shuffle_coeffs_array[] = {2,1,0,3, 6,5,4,7, 10,9,8,11, 14,13,12,15};
    uint8x16_t neon_shuffle_mask = vld1q_u8(shuffle_coeffs_array);

    while (num_pixels_to_process >= pixels_per_step) {
        uint8x16_t bgra_pixels_loaded = vld1q_u8(reinterpret_cast<const uint8_t*>(src_bgra_pixels + current_pixel_idx));
        uint8x16_t rgba_pixels = vqtbl1q_u8(bgra_pixels_loaded, neon_shuffle_mask);
        vst1q_u8(dest_rgba_data + current_pixel_idx * 4, rgba_pixels);
        current_pixel_idx += pixels_per_step;
        num_pixels_to_process -= pixels_per_step;
    }
#endif
    // Scalar fallback for remaining pixels in the row
    for (size_t i = 0; i < num_pixels_to_process; ++i) {
        const ::Pixel& src_pixel = *(src_bgra_pixels + current_pixel_idx + i);
        uint8_t* dest_pixel_ptr = dest_rgba_data + (current_pixel_idx + i) * 4;
        dest_pixel_ptr[0] = src_pixel.red;   // R
        dest_pixel_ptr[1] = src_pixel.green; // G
        dest_pixel_ptr[2] = src_pixel.blue;  // B
        dest_pixel_ptr[3] = src_pixel.alpha; // A
    }
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

    // 1. Perform input validation on bitmap_in
    if (bitmap_in.w == 0 || bitmap_in.h == 0) {
        return BitmapError::InvalidImageData;
    }
    if (bitmap_in.bpp != 32) {
        return BitmapError::UnsupportedBpp; // Expects 32bpp RGBA input
    }
    const size_t expected_input_data_size = static_cast<size_t>(bitmap_in.w) * bitmap_in.h * 4; // 4 bytes for RGBA
    if (bitmap_in.data.size() < expected_input_data_size) {
        return BitmapError::InvalidImageData; // Not enough pixel data provided
    }

    // 2. Convert BmpTool::Bitmap (RGBA) to Matrix::Matrix<::Pixel> (BGRA)
    Matrix::Matrix<::Pixel> image_matrix(bitmap_in.h, bitmap_in.w); // Matrix constructor is (rows, cols)
    for (uint32_t y = 0; y < bitmap_in.h; ++y) {
        const uint8_t* src_rgba_data_row = &bitmap_in.data[(static_cast<size_t>(y) * bitmap_in.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bitmap_in.w);
    }

    // 3. Call ::CreateBitmapFromMatrix
    ::Bitmap::File temp_bmp_file = ::CreateBitmapFromMatrix(image_matrix);

    // 4. If !temp_bmp_file.IsValid(), return BitmapError::UnknownError
    if (!temp_bmp_file.IsValid()) {
        return BitmapError::UnknownError; // Error during CreateBitmapFromMatrix
    }

    // 5. Calculate total_required_size
    // Ensure bfSize in the header is correct. CreateBitmapFromMatrix should set this.
    // Also, bfOffBits should be correctly set by CreateBitmapFromMatrix.
    // We rely on temp_bmp_file.bitmapData.size() for the pixel data size.
    uint32_t total_required_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + temp_bmp_file.bitmapData.size();

    // As an integrity check, bfSize from the library should match calculated total size
    if (temp_bmp_file.bitmapFileHeader.bfSize != total_required_size) {
       // This might indicate an internal issue with CreateBitmapFromMatrix or a misunderstanding of its output.
       // For robustness, one might choose to trust total_required_size or return an error.
       // Given the instructions, we proceed with total_required_size for buffer check.
       // Optionally: temp_bmp_file.bitmapFileHeader.bfSize = total_required_size;
       // And: temp_bmp_file.bitmapFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    }


    // 6. If out_bmp_buffer.size() < total_required_size, return BitmapError::OutputBufferTooSmall
    if (out_bmp_buffer.size() < total_required_size) {
        return BitmapError::OutputBufferTooSmall;
    }

    // 7. Copy temp_bmp_file.bitmapFileHeader, temp_bmp_file.bitmapInfoHeader, and temp_bmp_file.bitmapData
    uint8_t* buffer_ptr = out_bmp_buffer.data();
    std::memcpy(buffer_ptr, &temp_bmp_file.bitmapFileHeader, sizeof(BITMAPFILEHEADER));
    buffer_ptr += sizeof(BITMAPFILEHEADER);
    std::memcpy(buffer_ptr, &temp_bmp_file.bitmapInfoHeader, sizeof(BITMAPINFOHEADER));
    buffer_ptr += sizeof(BITMAPINFOHEADER);

    if (!temp_bmp_file.bitmapData.empty()) {
        std::memcpy(buffer_ptr, temp_bmp_file.bitmapData.data(), temp_bmp_file.bitmapData.size());
    } else if (total_required_size > (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER))) {
        // This case (empty data but headers imply data) should ideally be caught by temp_bmp_file.IsValid()
        // or result in temp_bmp_file.bitmapData.size() being consistent.
        // If CreateBitmapFromMatrix produced such a state and marked it valid, it's an issue.
        return BitmapError::InvalidImageData; // Or UnknownError from CreateBitmapFromMatrix inconsistency
    }

    // 8. Return BmpTool::Success{}
    return BmpTool::Success{};
}

// Helper function to convert an array of uint8_t (RGBA order) to an array of ::Pixel (BGRA order)
void internal_swizzle_rgba_to_bgra_simd(const uint8_t* src_rgba_data, ::Pixel* dest_bgra_pixels, size_t num_pixels) {
    size_t current_pixel_idx = 0;
    size_t num_pixels_to_process = num_pixels;

#if defined(__AVX2__)
    const size_t pixels_per_step = 8; // 8 pixels = 32 bytes
    __m256i shuffle_mask_rgba_to_bgra = _mm256_setr_epi8(
        2, 1, 0, 3,  6, 5, 4, 7,  10, 9, 8, 11,  14,13,12,15,
        18,17,16,19, 22,21,20,23, 26,25,24,27, 30,29,28,31
    );
    while (num_pixels_to_process >= pixels_per_step) {
        __m256i rgba_pixels_loaded = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src_rgba_data + current_pixel_idx * 4));
        __m256i bgra_pixels = _mm256_shuffle_epi8(rgba_pixels_loaded, shuffle_mask_rgba_to_bgra);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(dest_bgra_pixels + current_pixel_idx), bgra_pixels);
        current_pixel_idx += pixels_per_step;
        num_pixels_to_process -= pixels_per_step;
    }
#elif defined(__SSSE3__) // _mm_shuffle_epi8 requires SSSE3
    const size_t pixels_per_step = 4; // 4 pixels = 16 bytes
    __m128i shuffle_mask_rgba_to_bgra = _mm_setr_epi8(
        2, 1, 0, 3,  6, 5, 4, 7,  10, 9, 8, 11,  14,13,12,15
    );
    while (num_pixels_to_process >= pixels_per_step) {
        __m128i rgba_pixels_loaded = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src_rgba_data + current_pixel_idx * 4));
        __m128i bgra_pixels = _mm_shuffle_epi8(rgba_pixels_loaded, shuffle_mask_rgba_to_bgra);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dest_bgra_pixels + current_pixel_idx), bgra_pixels);
        current_pixel_idx += pixels_per_step;
        num_pixels_to_process -= pixels_per_step;
    }
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
    const size_t pixels_per_step = 4; // 4 pixels = 16 bytes
    const uint8_t shuffle_coeffs_array[] = {2,1,0,3, 6,5,4,7, 10,9,8,11, 14,13,12,15};
    uint8x16_t neon_shuffle_mask = vld1q_u8(shuffle_coeffs_array);

    while (num_pixels_to_process >= pixels_per_step) {
        uint8x16_t rgba_pixels_loaded = vld1q_u8(src_rgba_data + current_pixel_idx * 4);
        uint8x16_t bgra_pixels = vqtbl1q_u8(rgba_pixels_loaded, neon_shuffle_mask);
        vst1q_u8(reinterpret_cast<uint8_t*>(dest_bgra_pixels + current_pixel_idx), bgra_pixels);
        current_pixel_idx += pixels_per_step;
        num_pixels_to_process -= pixels_per_step;
    }
#endif
    // Scalar fallback for remaining pixels in the row
    for (size_t i = 0; i < num_pixels_to_process; ++i) {
        const uint8_t* src_pixel_ptr = src_rgba_data + (current_pixel_idx + i) * 4;
        ::Pixel& dest_pixel = *(dest_bgra_pixels + current_pixel_idx + i);
        
        dest_pixel.red   = src_pixel_ptr[0]; // R
        dest_pixel.green = src_pixel_ptr[1]; // G
        dest_pixel.blue  = src_pixel_ptr[2]; // B
        dest_pixel.alpha = src_pixel_ptr[3]; // A
    }
}

// Implementation of image manipulation functions

Result<Bitmap, BitmapError> shrink(const Bitmap& bmp_tool_bitmap, int scaleFactor) {
    // 1. Validate input BmpTool::Bitmap and other parameters
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }
    if (scaleFactor <= 0) {
        return BitmapError::InvalidImageData; // scaleFactor must be positive
    }

    // 2. Convert BmpTool::Bitmap (RGBA) to ::Bitmap::File (via Matrix::Matrix<::Pixel> BGRA)
    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) {
        return BitmapError::UnknownError;
    }

    // 3. Call the core function from src/bitmap/bitmap.cpp
    ::Bitmap::File result_core_bitmap_file = ::ShrinkImage(core_bitmap_file, scaleFactor);

    if (!result_core_bitmap_file.IsValid()) {
        // ShrinkImage might return an invalid/empty bitmap if scaleFactor is too large,
        // leading to zero width/height. This is handled by the conversion step below.
        // If it's invalid for other reasons, it's an UnknownError.
        if (result_core_bitmap_file.bitmapInfoHeader.biWidth == 0 || result_core_bitmap_file.bitmapInfoHeader.biHeight == 0){
            // This is a valid outcome for shrink, will result in an empty BmpTool::Bitmap
        } else {
            return BitmapError::UnknownError; // Core operation failed for other reasons
        }
    }

    // 4. Convert resulting ::Bitmap::File back to BmpTool::Bitmap (RGBA)
    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);

    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
            return BitmapError::InvalidImageData; // Output image dimensions too large
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    // If w or h is 0, data remains empty, which is correct.

    // 5. Return
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> rotateCounterClockwise(const Bitmap& bmp_tool_bitmap) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::RotateImageCounterClockwise(core_bitmap_file);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> rotateClockwise(const Bitmap& bmp_tool_bitmap) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::RotateImageClockwise(core_bitmap_file);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> mirror(const Bitmap& bmp_tool_bitmap) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::MirrorImage(core_bitmap_file);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> flip(const Bitmap& bmp_tool_bitmap) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::FlipImage(core_bitmap_file);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> greyscale(const Bitmap& bmp_tool_bitmap) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::GreyscaleImage(core_bitmap_file);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> changeBrightness(const Bitmap& bmp_tool_bitmap, float brightness) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::ChangeImageBrightness(core_bitmap_file, brightness);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> changeContrast(const Bitmap& bmp_tool_bitmap, float contrast) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::ChangeImageContrast(core_bitmap_file, contrast);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> changeSaturation(const Bitmap& bmp_tool_bitmap, float saturation) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::ChangeImageSaturation(core_bitmap_file, saturation);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> changeSaturationBlue(const Bitmap& bmp_tool_bitmap, float saturation) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::ChangeImageSaturationBlue(core_bitmap_file, saturation);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> changeSaturationGreen(const Bitmap& bmp_tool_bitmap, float saturation) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::ChangeImageSaturationGreen(core_bitmap_file, saturation);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> changeSaturationRed(const Bitmap& bmp_tool_bitmap, float saturation) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::ChangeImageSaturationRed(core_bitmap_file, saturation);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> changeSaturationMagenta(const Bitmap& bmp_tool_bitmap, float saturation) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::ChangeImageSaturationMagenta(core_bitmap_file, saturation);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> changeSaturationYellow(const Bitmap& bmp_tool_bitmap, float saturation) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::ChangeImageSaturationYellow(core_bitmap_file, saturation);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> changeSaturationCyan(const Bitmap& bmp_tool_bitmap, float saturation) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::ChangeImageSaturationCyan(core_bitmap_file, saturation);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> changeLuminanceBlue(const Bitmap& bmp_tool_bitmap, float luminance) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::ChangeImageLuminanceBlue(core_bitmap_file, luminance);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> changeLuminanceGreen(const Bitmap& bmp_tool_bitmap, float luminance) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::ChangeImageLuminanceGreen(core_bitmap_file, luminance);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> changeLuminanceRed(const Bitmap& bmp_tool_bitmap, float luminance) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::ChangeImageLuminanceRed(core_bitmap_file, luminance);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> changeLuminanceMagenta(const Bitmap& bmp_tool_bitmap, float luminance) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::ChangeImageLuminanceMagenta(core_bitmap_file, luminance);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> changeLuminanceYellow(const Bitmap& bmp_tool_bitmap, float luminance) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::ChangeImageLuminanceYellow(core_bitmap_file, luminance);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> changeLuminanceCyan(const Bitmap& bmp_tool_bitmap, float luminance) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::ChangeImageLuminanceCyan(core_bitmap_file, luminance);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> invertColors(const Bitmap& bmp_tool_bitmap) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::InvertImageColors(core_bitmap_file);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> applySepiaTone(const Bitmap& bmp_tool_bitmap) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::ApplySepiaToneToImage(core_bitmap_file);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

Result<Bitmap, BitmapError> applyBoxBlur(const Bitmap& bmp_tool_bitmap, int blurRadius) {
    if (bmp_tool_bitmap.w == 0 || bmp_tool_bitmap.h == 0 || bmp_tool_bitmap.bpp != 32 ||
        bmp_tool_bitmap.data.size() < static_cast<size_t>(bmp_tool_bitmap.w) * bmp_tool_bitmap.h * 4) {
        return BitmapError::InvalidImageData;
    }
    if (blurRadius < 0) {
        return BitmapError::InvalidImageData; // blurRadius must be non-negative
    }

    Matrix::Matrix<::Pixel> image_matrix(bmp_tool_bitmap.h, bmp_tool_bitmap.w);
    for (uint32_t y = 0; y < bmp_tool_bitmap.h; ++y) {
        const uint8_t* src_rgba_data_row = &bmp_tool_bitmap.data[(static_cast<size_t>(y) * bmp_tool_bitmap.w * 4)];
        ::Pixel* dest_bgra_pixels_row = &image_matrix[y][0];
        internal_swizzle_rgba_to_bgra_simd(src_rgba_data_row, dest_bgra_pixels_row, bmp_tool_bitmap.w);
    }

    ::Bitmap::File core_bitmap_file = ::CreateBitmapFromMatrix(image_matrix);
    if (!core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    ::Bitmap::File result_core_bitmap_file = ::ApplyBoxBlurToImage(core_bitmap_file, blurRadius);
    if (!result_core_bitmap_file.IsValid()) { return BitmapError::UnknownError; }

    Matrix::Matrix<::Pixel> result_image_matrix = ::CreateMatrixFromBitmap(result_core_bitmap_file);
    Bitmap final_bmp_tool_bitmap;
    final_bmp_tool_bitmap.w = result_image_matrix.cols();
    final_bmp_tool_bitmap.h = result_image_matrix.rows();
    final_bmp_tool_bitmap.bpp = 32;

    if (final_bmp_tool_bitmap.w > 0 && final_bmp_tool_bitmap.h > 0) {
        if (final_bmp_tool_bitmap.h > 0 && final_bmp_tool_bitmap.w > (std::numeric_limits<size_t>::max() / final_bmp_tool_bitmap.h / 4)) {
             return BitmapError::InvalidImageData;
        }
        final_bmp_tool_bitmap.data.resize(static_cast<size_t>(final_bmp_tool_bitmap.w) * final_bmp_tool_bitmap.h * 4);
        for (uint32_t y = 0; y < final_bmp_tool_bitmap.h; ++y) {
            const ::Pixel* src_bgra_pixels_row = &result_image_matrix[y][0];
            uint8_t* dest_rgba_data_row = &final_bmp_tool_bitmap.data[(static_cast<size_t>(y) * final_bmp_tool_bitmap.w * 4)];
            internal_swizzle_bgra_to_rgba_simd(src_bgra_pixels_row, dest_rgba_data_row, final_bmp_tool_bitmap.w);
        }
    }
    return final_bmp_tool_bitmap;
}

} // namespace BmpTool
