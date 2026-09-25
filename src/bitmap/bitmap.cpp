#include "bitmap.h"
#include <iostream>  // For standard I/O (though not explicitly used in this file's current state).
#include <algorithm> // For std::min and std::max, used in ApplyBoxBlur and color adjustments.
#include <vector>    // For std::vector, used by Matrix class and underlying bitmap data.
#include <cstring>   // For std::memcpy, used in SIMD NEON path
#include <cmath>     // For std::isfinite
#include "../simd_utils.hpp" // Added include
#include "../safe_math.hpp"

// Define BI_RGB as 0 if not already defined, to ensure cross-platform compatibility for bitmap compression type.
#ifndef BI_RGB
#define BI_RGB 0
#endif

// Captures a screenshot of a specified window and returns it as a Bitmap::File object.
// This function uses Windows API calls to interact with window handles and device contexts.
#ifdef _WIN32
Bitmap::File ScreenShotWindow(HWND windowHandle)
{
    Bitmap::File bitmapFile; // Structure to hold bitmap data and headers.

    // Prepare window for capture
    OpenIcon(windowHandle);             // If window is minimized, restore it.
    BringWindowToTop(windowHandle);     // Bring the window to the foreground.
    SetActiveWindow(windowHandle);      // Set the window as active.

    // Get device context of the window.
    HDC deviceContextHandle = GetDC(windowHandle);
    // Create a compatible device context (DC) for the bitmap.
    HDC deviceContext = CreateCompatibleDC(deviceContextHandle);

    RECT windowRectangle;
    GetClientRect(windowHandle, &windowRectangle); // Get the dimensions of the client area of the window.

    // Create a compatible bitmap with the dimensions of the window's client area.
    HBITMAP bitmapHandle = CreateCompatibleBitmap(deviceContextHandle, windowRectangle.right, windowRectangle.bottom);
    BITMAP deviceContextBitmap; // Structure to hold bitmap metadata.

    // Select the bitmap into the compatible DC.
    SelectObject(deviceContext, bitmapHandle);

    // Copy the screen data from the window's DC to the compatible DC (and thus to the bitmap).
    // SRCCOPY indicates a direct copy of the source to the destination.
    BitBlt(deviceContext, 0, 0, windowRectangle.right, windowRectangle.bottom, deviceContextHandle, 0, 0, SRCCOPY);

    // Retrieve metadata about the created bitmap.
    int objectGotSuccessfully = GetObject(bitmapHandle, sizeof(BITMAP), &deviceContextBitmap);

    if (objectGotSuccessfully)
    {
        // Populate BITMAPINFOHEADER
        bitmapFile.bitmapInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapFile.bitmapInfoHeader.biWidth = deviceContextBitmap.bmWidth;
        bitmapFile.bitmapInfoHeader.biHeight = deviceContextBitmap.bmHeight; // Positive height for bottom-up DIB.
        bitmapFile.bitmapInfoHeader.biPlanes = deviceContextBitmap.bmPlanes; // Usually 1.
        bitmapFile.bitmapInfoHeader.biBitCount = deviceContextBitmap.bmBitsPixel; // Bits per pixel (e.g., 24 or 32).
        bitmapFile.bitmapInfoHeader.biCompression = BI_RGB; // Uncompressed RGB.
        // Calculate image size in bytes. For BI_RGB, this can be 0 if biHeight is positive.
        // However, explicitly calculating it is safer for raw data access.
        int imageSize = deviceContextBitmap.bmWidth * deviceContextBitmap.bmHeight * (deviceContextBitmap.bmBitsPixel / 8);
        bitmapFile.bitmapInfoHeader.biSizeImage = imageSize; // Total size of the image data.
        // Resize the vector to hold the pixel data.
        bitmapFile.bitmapData.resize(imageSize);

        // Populate BITMAPFILEHEADER
        int offsetSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); // Offset to pixel data.
        int fileSize = offsetSize + imageSize; // Total file size.
        bitmapFile.bitmapFileHeader.bfSize = fileSize;
        bitmapFile.bitmapFileHeader.bfType = 0x4D42; // BM signature for bitmap files.
        bitmapFile.bitmapFileHeader.bfOffBits = offsetSize;
        bitmapFile.bitmapFileHeader.bfReserved1 = 0;
        bitmapFile.bitmapFileHeader.bfReserved2 = 0;

        BITMAPINFO bmi = {};
        bmi.bmiHeader = bitmapFile.bitmapInfoHeader;
        int DIBitsGotSuccessfully = GetDIBits(deviceContextHandle, bitmapHandle, 0, deviceContextBitmap.bmHeight, &bitmapFile.bitmapData[0], &bmi, DIB_RGB_COLORS);
        if (DIBitsGotSuccessfully)
            bitmapFile.SetValid(); // Mark the bitmap file as valid.
    }

    // Clean up GDI objects.
    DeleteDC(deviceContext);           // Delete the compatible DC.
    ReleaseDC(windowHandle, deviceContextHandle); // Release the window's DC.
    DeleteObject(bitmapHandle);        // Delete the bitmap object.

    return bitmapFile;
}
#endif // _WIN32

// Inverts the color of a single pixel (Red, Green, Blue channels). Alpha is unchanged.
Pixel InvertPixelColor(Pixel pixel)
{
    pixel.red = 255 - pixel.red;
    pixel.green = 255 - pixel.green;
    pixel.blue = 255 - pixel.blue;
    // Alpha remains unchanged
    return pixel;
}

// Inverts the colors of the image.
Bitmap::File InvertImageColors(Bitmap::File bitmapFile)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (int i = 0; i < imageMatrix.rows(); ++i)
        for (int j = 0; j < imageMatrix.cols(); ++j)
            imageMatrix[i][j] = InvertPixelColor(imageMatrix[i][j]);
    return CreateBitmapFromMatrix(imageMatrix);
}

// Applies sepia tone to a single pixel.
Pixel ApplySepiaToPixel(Pixel pixel)
{
    // Standard sepia calculation
    unsigned int tr = (unsigned int)(0.393 * pixel.red + 0.769 * pixel.green + 0.189 * pixel.blue);
    unsigned int tg = (unsigned int)(0.349 * pixel.red + 0.686 * pixel.green + 0.168 * pixel.blue);
    unsigned int tb = (unsigned int)(0.272 * pixel.red + 0.534 * pixel.green + 0.131 * pixel.blue);

    pixel.red = (tr > 255) ? 255 : (BYTE)tr;
    pixel.green = (tg > 255) ? 255 : (BYTE)tg;
    pixel.blue = (tb > 255) ? 255 : (BYTE)tb;
    // Alpha remains unchanged
    return pixel;
}

// Applies a sepia tone to the image.
Bitmap::File ApplySepiaTone(Bitmap::File bitmapFile)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (int i = 0; i < imageMatrix.rows(); ++i)
        for (int j = 0; j < imageMatrix.cols(); ++j)
            imageMatrix[i][j] = ApplySepiaToPixel(imageMatrix[i][j]);
    return CreateBitmapFromMatrix(imageMatrix);
}

// Applies a box blur to the image with a given radius.
// Each pixel's new value is the average of its neighbors within a square box.
Bitmap::File ApplyBoxBlur(Bitmap::File bitmapFile, int blurRadius)
{
    if (blurRadius <= 0) {
        // No blur or invalid radius, return original image without processing.
        return bitmapFile;
    }
    blurRadius = BmpTool::SafeMath::clamp(blurRadius, 0, BmpTool::SafeMath::MAX_SAFE_BLUR_RADIUS);

    Matrix::Matrix<Pixel> originalMatrix = CreateMatrixFromBitmap(bitmapFile);
    Matrix::Matrix<Pixel> blurredMatrix(originalMatrix.rows(), originalMatrix.cols());

    // Iterate over each pixel in the original image.
    for (int r = 0; r < static_cast<int>(originalMatrix.rows()); ++r) // r for row
    {
        for (int c = 0; c < static_cast<int>(originalMatrix.cols()); ++c) // c for column
        {
            unsigned int sumRed = 0, sumGreen = 0, sumBlue = 0, sumAlpha = 0;
            int count = 0; // Number of pixels included in the blur box.

            // Iterate over the box defined by blurRadius around the current pixel (r, c).
            // std::max and std::min are used to handle boundary conditions, ensuring we don't go out of bounds.
            for (int i = std::max(0, r - blurRadius); i <= std::min(r + blurRadius, static_cast<int>(originalMatrix.rows()) - 1); ++i)
            {
                for (int j = std::max(0, c - blurRadius); j <= std::min(c + blurRadius, static_cast<int>(originalMatrix.cols()) - 1); ++j)
                {
                    // Accumulate color and alpha values.
                    sumRed += originalMatrix[i][j].red;
                    sumGreen += originalMatrix[i][j].green;
                    sumBlue += originalMatrix[i][j].blue;
                    sumAlpha += originalMatrix[i][j].alpha;
                    count++;
                }
            }

            // Calculate the average color and alpha values.
            if (count > 0)
            {
                blurredMatrix[r][c].red = (BYTE)(sumRed / count);
                blurredMatrix[r][c].green = (BYTE)(sumGreen / count);
                blurredMatrix[r][c].blue = (BYTE)(sumBlue / count);
                blurredMatrix[r][c].alpha = (BYTE)(sumAlpha / count); // Blur alpha channel as well.
            }
            else
            {
                // This case should ideally not be reached if blurRadius > 0 and matrix is not empty.
                // As a fallback, copy the original pixel to the blurred matrix.
                blurredMatrix[r][c] = originalMatrix[r][c];
            }
        }
    }
    // Convert the matrix of blurred pixels back to a Bitmap::File object.
    return CreateBitmapFromMatrix(blurredMatrix);
}

// Applies a non-linear median filter to the image with a given kernel size (3 or 5).
// Each pixel's new channel value is the median of its neighbors within a square kernel.
Bitmap::File ApplyMedianFilter(Bitmap::File bitmapFile, int kernelSize)
{
    if (kernelSize != 3 && kernelSize != 5) {
        // Only 3x3 and 5x5 median filters are supported; return original image on invalid size.
        return bitmapFile;
    }

    Matrix::Matrix<Pixel> originalMatrix = CreateMatrixFromBitmap(bitmapFile);
    if (originalMatrix.rows() == 0 || originalMatrix.cols() == 0) {
        return bitmapFile;
    }

    Matrix::Matrix<Pixel> filteredMatrix(originalMatrix.rows(), originalMatrix.cols());
    const int radius = kernelSize / 2;
    const int numNeighbors = kernelSize * kernelSize;
    const int medianIndex = numNeighbors / 2;

    uint8_t rBuf[25];
    uint8_t gBuf[25];
    uint8_t bBuf[25];
    uint8_t aBuf[25];

    for (int r = 0; r < static_cast<int>(originalMatrix.rows()); ++r)
    {
        for (int c = 0; c < static_cast<int>(originalMatrix.cols()); ++c)
        {
            int idx = 0;
            for (int dr = -radius; dr <= radius; ++dr)
            {
                int sample_r = std::clamp(r + dr, 0, static_cast<int>(originalMatrix.rows()) - 1);
                for (int dc = -radius; dc <= radius; ++dc)
                {
                    int sample_c = std::clamp(c + dc, 0, static_cast<int>(originalMatrix.cols()) - 1);
                    const Pixel& p = originalMatrix[sample_r][sample_c];
                    rBuf[idx] = p.red;
                    gBuf[idx] = p.green;
                    bBuf[idx] = p.blue;
                    aBuf[idx] = p.alpha;
                    idx++;
                }
            }

            std::nth_element(rBuf, rBuf + medianIndex, rBuf + numNeighbors);
            std::nth_element(gBuf, gBuf + medianIndex, gBuf + numNeighbors);
            std::nth_element(bBuf, bBuf + medianIndex, bBuf + numNeighbors);
            std::nth_element(aBuf, aBuf + medianIndex, aBuf + numNeighbors);

            filteredMatrix[r][c].red = rBuf[medianIndex];
            filteredMatrix[r][c].green = gBuf[medianIndex];
            filteredMatrix[r][c].blue = bBuf[medianIndex];
            filteredMatrix[r][c].alpha = aBuf[medianIndex];
        }
    }
    return CreateBitmapFromMatrix(filteredMatrix);
}

// Applies a Gaussian blur filter to the image with a given sigma and radius.
Bitmap::File ApplyGaussianBlur(Bitmap::File bitmapFile, float sigma, int radius)
{
    if (!std::isfinite(sigma) || sigma <= 0.0f) {
        return bitmapFile;
    }
    if (radius < 0) {
        return bitmapFile;
    }
    if (radius == 0) {
        radius = static_cast<int>(std::ceil(3.0f * sigma));
    }
    radius = BmpTool::SafeMath::clamp(radius, 0, BmpTool::SafeMath::MAX_SAFE_BLUR_RADIUS);
    if (radius == 0) {
        return bitmapFile;
    }

    Matrix::Matrix<Pixel> originalMatrix = CreateMatrixFromBitmap(bitmapFile);
    if (originalMatrix.rows() == 0 || originalMatrix.cols() == 0) {
        return bitmapFile;
    }

    const int kernelSize = 2 * radius + 1;
    std::vector<float> kernel(kernelSize);
    const float twoSigmaSq = 2.0f * sigma * sigma;
    float sumWeights = 0.0f;
    for (int i = -radius; i <= radius; ++i) {
        float w = std::exp(-static_cast<float>(i * i) / twoSigmaSq);
        kernel[i + radius] = w;
        sumWeights += w;
    }
    for (float& w : kernel) {
        w /= sumWeights;
    }

    const int rows = static_cast<int>(originalMatrix.rows());
    const int cols = static_cast<int>(originalMatrix.cols());

    // Separable Pass 1: Horizontal blur into intermediate float buffer
    struct FPixel { float r, g, b, a; };
    std::vector<FPixel> temp(static_cast<size_t>(rows) * cols);

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            float red = 0.0f, green = 0.0f, blue = 0.0f, alpha = 0.0f;
            for (int k = -radius; k <= radius; ++k) {
                int sc = std::clamp(c + k, 0, cols - 1);
                const Pixel& p = originalMatrix[r][sc];
                float w = kernel[k + radius];
                red += p.red * w;
                green += p.green * w;
                blue += p.blue * w;
                alpha += p.alpha * w;
            }
            temp[static_cast<size_t>(r) * cols + c] = {red, green, blue, alpha};
        }
    }

    // Separable Pass 2: Vertical blur from temp into blurredMatrix
    Matrix::Matrix<Pixel> blurredMatrix(rows, cols);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            float red = 0.0f, green = 0.0f, blue = 0.0f, alpha = 0.0f;
            for (int k = -radius; k <= radius; ++k) {
                int sr = std::clamp(r + k, 0, rows - 1);
                const FPixel& p = temp[static_cast<size_t>(sr) * cols + c];
                float w = kernel[k + radius];
                red += p.r * w;
                green += p.g * w;
                blue += p.b * w;
                alpha += p.a * w;
            }
            blurredMatrix[r][c].red   = static_cast<BYTE>(std::clamp(std::round(red), 0.0f, 255.0f));
            blurredMatrix[r][c].green = static_cast<BYTE>(std::clamp(std::round(green), 0.0f, 255.0f));
            blurredMatrix[r][c].blue  = static_cast<BYTE>(std::clamp(std::round(blue), 0.0f, 255.0f));
            blurredMatrix[r][c].alpha = static_cast<BYTE>(std::clamp(std::round(alpha), 0.0f, 255.0f));
        }
    }

    return CreateBitmapFromMatrix(blurredMatrix);
}

// Converts the image to grayscale using ITU-R BT.709 or BT.601 photometric luma weighting.
Bitmap::File ConvertToPhotometricLuma(Bitmap::File bitmapFile, bool useBT709)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    const int rows = static_cast<int>(imageMatrix.rows());
    const int cols = static_cast<int>(imageMatrix.cols());
    if (rows == 0 || cols == 0) {
        return bitmapFile;
    }

    const uint32_t wR = useBT709 ? 13933u : 19595u;
    const uint32_t wG = useBT709 ? 46871u : 38470u;
    const uint32_t wB = useBT709 ? 4732u  : 7471u;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            Pixel& p = imageMatrix[r][c];
            uint32_t luma = (wR * p.red + wG * p.green + wB * p.blue + 32768u) >> 16;
            BYTE y = static_cast<BYTE>(std::clamp(luma, 0u, 255u));
            p.red = y;
            p.green = y;
            p.blue = y;
        }
    }
    return CreateBitmapFromMatrix(imageMatrix);
}

// Applies Local Contrast Normalization (LCN) to the image.
Bitmap::File ApplyLocalContrastNormalization(Bitmap::File bitmapFile, float sigma, float alpha, float epsilon)
{
    if (!std::isfinite(sigma) || sigma <= 0.0f ||
        !std::isfinite(alpha) || alpha <= 0.0f ||
        !std::isfinite(epsilon) || epsilon <= 0.0f) {
        return bitmapFile;
    }

    Matrix::Matrix<Pixel> originalMatrix = CreateMatrixFromBitmap(bitmapFile);
    const int rows = static_cast<int>(originalMatrix.rows());
    const int cols = static_cast<int>(originalMatrix.cols());
    if (rows == 0 || cols == 0) {
        return bitmapFile;
    }

    int radius = static_cast<int>(std::ceil(3.0f * sigma));
    radius = BmpTool::SafeMath::clamp(radius, 1, BmpTool::SafeMath::MAX_SAFE_BLUR_RADIUS);

    const int kernelSize = 2 * radius + 1;
    std::vector<float> kernel(kernelSize);
    const float twoSigmaSq = 2.0f * sigma * sigma;
    float sumWeights = 0.0f;
    for (int i = -radius; i <= radius; ++i) {
        float w = std::exp(-static_cast<float>(i * i) / twoSigmaSq);
        kernel[i + radius] = w;
        sumWeights += w;
    }
    for (float& w : kernel) {
        w /= sumWeights;
    }

    auto blurFloatGrid = [&](const std::vector<float>& input) -> std::vector<float> {
        std::vector<float> temp(static_cast<size_t>(rows) * cols);
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                float val = 0.0f;
                for (int k = -radius; k <= radius; ++k) {
                    int sc = std::clamp(c + k, 0, cols - 1);
                    val += input[static_cast<size_t>(r) * cols + sc] * kernel[k + radius];
                }
                temp[static_cast<size_t>(r) * cols + c] = val;
            }
        }
        std::vector<float> result(static_cast<size_t>(rows) * cols);
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                float val = 0.0f;
                for (int k = -radius; k <= radius; ++k) {
                    int sr = std::clamp(r + k, 0, rows - 1);
                    val += temp[static_cast<size_t>(sr) * cols + c] * kernel[k + radius];
                }
                result[static_cast<size_t>(r) * cols + c] = val;
            }
        }
        return result;
    };

    const size_t numPixels = static_cast<size_t>(rows) * cols;
    std::vector<float> inR(numPixels), inG(numPixels), inB(numPixels);
    std::vector<float> inR2(numPixels), inG2(numPixels), inB2(numPixels);

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const size_t idx = static_cast<size_t>(r) * cols + c;
            const Pixel& p = originalMatrix[r][c];
            float vr = static_cast<float>(p.red);
            float vg = static_cast<float>(p.green);
            float vb = static_cast<float>(p.blue);
            inR[idx]  = vr;
            inR2[idx] = vr * vr;
            inG[idx]  = vg;
            inG2[idx] = vg * vg;
            inB[idx]  = vb;
            inB2[idx] = vb * vb;
        }
    }

    std::vector<float> meanR  = blurFloatGrid(inR);
    std::vector<float> meanR2 = blurFloatGrid(inR2);
    std::vector<float> meanG  = blurFloatGrid(inG);
    std::vector<float> meanG2 = blurFloatGrid(inG2);
    std::vector<float> meanB  = blurFloatGrid(inB);
    std::vector<float> meanB2 = blurFloatGrid(inB2);

    Matrix::Matrix<Pixel> normalizedMatrix(rows, cols);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const size_t idx = static_cast<size_t>(r) * cols + c;
            const Pixel& p = originalMatrix[r][c];

            float varR = std::max(0.0f, meanR2[idx] - meanR[idx] * meanR[idx]);
            float devR = std::sqrt(varR) + epsilon;
            float normR = 128.0f + alpha * (static_cast<float>(p.red) - meanR[idx]) / devR;

            float varG = std::max(0.0f, meanG2[idx] - meanG[idx] * meanG[idx]);
            float devG = std::sqrt(varG) + epsilon;
            float normG = 128.0f + alpha * (static_cast<float>(p.green) - meanG[idx]) / devG;

            float varB = std::max(0.0f, meanB2[idx] - meanB[idx] * meanB[idx]);
            float devB = std::sqrt(varB) + epsilon;
            float normB = 128.0f + alpha * (static_cast<float>(p.blue) - meanB[idx]) / devB;

            normalizedMatrix[r][c].red   = static_cast<BYTE>(std::clamp(std::round(normR), 0.0f, 255.0f));
            normalizedMatrix[r][c].green = static_cast<BYTE>(std::clamp(std::round(normG), 0.0f, 255.0f));
            normalizedMatrix[r][c].blue  = static_cast<BYTE>(std::clamp(std::round(normB), 0.0f, 255.0f));
            normalizedMatrix[r][c].alpha = p.alpha;
        }
    }

    return CreateBitmapFromMatrix(normalizedMatrix);
}




// Helper function for BGR to BGRA conversion with SIMD (declaration in bitmap.h)
void internal_convert_bgr_to_bgra_simd(const uint8_t* src_row_bgr_ptr, ::Pixel* dest_row_pixel_ptr, size_t num_pixels_in_row) {
    size_t current_src_byte_offset = 0;
    size_t current_dest_pixel_idx = 0;
    size_t num_pixels_to_process = num_pixels_in_row;

#if defined(__AVX2__)
    // As per previous implementation, AVX2 uses SSSE3 logic.
    // No distinct 256-bit AVX2 path implemented here.
#endif

#if defined(__AVX2__) || defined(__SSSE3__) // Use SSSE3 for AVX2 as well if no specific AVX2 code
    const size_t pixels_per_step = 4; 
    __m128i bgr_to_bgrX_mask = _mm_setr_epi8(
        0, 1, 2, (char)0x80, 
        3, 4, 5, (char)0x80, 
        6, 7, 8, (char)0x80, 
        9, 10, 11, (char)0x80 
    );
    __m128i alpha_channel_ff = _mm_setr_epi8(
        0,0,0, (char)0xFF, 0,0,0,(char)0xFF, 0,0,0,(char)0xFF, 0,0,0,(char)0xFF
    );

    while (num_pixels_to_process >= pixels_per_step) {
        __m128i bgr_data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src_row_bgr_ptr + current_src_byte_offset));
        __m128i bgra_pixels_expanded = _mm_shuffle_epi8(bgr_data, bgr_to_bgrX_mask);
        __m128i bgra_pixels_final = _mm_or_si128(bgra_pixels_expanded, alpha_channel_ff);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dest_row_pixel_ptr + current_dest_pixel_idx), bgra_pixels_final);
        current_src_byte_offset += pixels_per_step * 3; 
        current_dest_pixel_idx += pixels_per_step;    
        num_pixels_to_process -= pixels_per_step;
    }
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
    const size_t pixels_per_step = 4;
    const uint8_t table_bgr_to_bgr0[] = {0,1,2,16, 3,4,5,16, 6,7,8,16, 9,10,11,16}; 
    uint8x16_t neon_shuffle_table = vld1q_u8(table_bgr_to_bgr0);
    const uint8_t alpha_bytes[] = {0,0,0,0xFF, 0,0,0,0xFF, 0,0,0,0xFF, 0,0,0,0xFF};
    uint8x16_t alpha_channel_ff_neon = vld1q_u8(alpha_bytes);
    uint8_t temp_bgr_load[16];

    while (num_pixels_to_process >= pixels_per_step) {
        std::memcpy(temp_bgr_load, src_row_bgr_ptr + current_src_byte_offset, 12);
        uint8x16_t bgr_data_loaded = vld1q_u8(temp_bgr_load);
        uint8x16_t bgra_expanded = vqtbl1q_u8(bgr_data_loaded, neon_shuffle_table);
        uint8x16_t bgra_final = vorrq_u8(bgra_expanded, alpha_channel_ff_neon);
        vst1q_u8(reinterpret_cast<uint8_t*>(dest_row_pixel_ptr + current_dest_pixel_idx), bgra_final);
        current_src_byte_offset += pixels_per_step * 3;
        current_dest_pixel_idx += pixels_per_step;
        num_pixels_to_process -= pixels_per_step;
    }
#else 
    // This #else block ensures that if no SIMD path is taken (e.g. SSSE3/NEON not defined, or AVX2 defined but its specific block is empty and it's not grouped with SSSE3),
    // the scalar loop below is the ONLY path for processing.
    // The current structure with #if defined(__AVX2__) || defined(__SSSE3__) followed by #elif defined(__ARM_NEON)
    // means this #else is for when NEITHER of those are true.
    // If AVX2 is defined, it uses the SSSE3 block. If only NEON is defined, it uses NEON block.
    // If none are defined, it falls to the scalar loop below.
#endif
    // Scalar fallback for remaining pixels OR if no SIMD defined/executed above
    for (size_t k_rem = 0; k_rem < num_pixels_to_process; ++k_rem) {
        (dest_row_pixel_ptr + current_dest_pixel_idx + k_rem)->blue  = *(src_row_bgr_ptr + current_src_byte_offset + k_rem * 3 + 0);
        (dest_row_pixel_ptr + current_dest_pixel_idx + k_rem)->green = *(src_row_bgr_ptr + current_src_byte_offset + k_rem * 3 + 1);
        (dest_row_pixel_ptr + current_dest_pixel_idx + k_rem)->red   = *(src_row_bgr_ptr + current_src_byte_offset + k_rem * 3 + 2);
        (dest_row_pixel_ptr + current_dest_pixel_idx + k_rem)->alpha = 255;
    }
}

// Converts a Bitmap::File object (containing raw bitmap data and headers)
// into a Matrix::Matrix<Pixel> for easier pixel manipulation.
Matrix::Matrix<Pixel> CreateMatrixFromBitmap(Bitmap::File bitmapFile)
{
    uint32_t abs_height = 0;
    if (!BmpTool::SafeMath::getSafeAbsoluteHeight(bitmapFile.bitmapInfoHeader.biHeight, abs_height)) {
        return Matrix::Matrix<Pixel>(0, 0);
    }
    if (bitmapFile.bitmapInfoHeader.biWidth <= 0 || 
        static_cast<uint32_t>(bitmapFile.bitmapInfoHeader.biWidth) > BmpTool::SafeMath::MAX_SAFE_DIMENSION) {
        return Matrix::Matrix<Pixel>(0, 0);
    }
    uint32_t width = static_cast<uint32_t>(bitmapFile.bitmapInfoHeader.biWidth);

    unsigned int bpp = bitmapFile.bitmapInfoHeader.biBitCount;
    if (bpp != 24 && bpp != 32) {
        return Matrix::Matrix<Pixel>(0, 0);
    }

    uint32_t expected_data_size = 0;
    if (!BmpTool::SafeMath::computePixelDataSize(width, abs_height, static_cast<uint16_t>(bpp), expected_data_size)) {
        return Matrix::Matrix<Pixel>(0, 0);
    }

    if (bitmapFile.bitmapData.size() < expected_data_size) {
        return Matrix::Matrix<Pixel>(0, 0);
    }

    Matrix::Matrix<Pixel> imageMatrix(abs_height, width);
    if (imageMatrix.rows() == 0 || imageMatrix.cols() == 0) {
        return imageMatrix;
    }

    if (bpp == 32) // For 32-bit bitmaps (BGRA)
    {
        size_t k = 0;
        for (size_t i = 0; i < imageMatrix.rows(); i++)
            for (size_t j = 0; j < imageMatrix.cols(); j++)
            {
                imageMatrix[i][j].blue = bitmapFile.bitmapData[k];
                imageMatrix[i][j].green = bitmapFile.bitmapData[k + 1];
                imageMatrix[i][j].red = bitmapFile.bitmapData[k + 2];
                imageMatrix[i][j].alpha = bitmapFile.bitmapData[k + 3];
                k += 4;
            }
    }
    else if (bpp == 24) // For 24-bit bitmaps (BGR)
    {
        uint32_t src_bytes_per_row = 0;
        if (!BmpTool::SafeMath::computeRowStride(width, 24, src_bytes_per_row)) {
            return Matrix::Matrix<Pixel>(0, 0);
        }

        for (size_t i = 0; i < imageMatrix.rows(); i++) {
            const uint8_t* src_row_bgr_ptr = bitmapFile.bitmapData.data() + (i * src_bytes_per_row);
            ::Pixel* dest_row_pixel_ptr = &imageMatrix[i][0];
            internal_convert_bgr_to_bgra_simd(src_row_bgr_ptr, dest_row_pixel_ptr, imageMatrix.cols());
        }
    }

    return imageMatrix;
}

// Converts a Matrix::Matrix<Pixel> (representing an image)
// back into a Bitmap::File object (with raw bitmap data and headers).
// Assumes output is always a 32-bit bitmap.
Bitmap::File CreateBitmapFromMatrix(const Matrix::Matrix<Pixel>& imageMatrix)
{
    Bitmap::File bitmapFile;

    if (imageMatrix.rows() == 0 || imageMatrix.cols() == 0 ||
        imageMatrix.rows() > BmpTool::SafeMath::MAX_SAFE_DIMENSION ||
        imageMatrix.cols() > BmpTool::SafeMath::MAX_SAFE_DIMENSION) {
        return bitmapFile;
    }

    size_t total_pixels = 0;
    if (!BmpTool::SafeMath::multiply(imageMatrix.rows(), imageMatrix.cols(), total_pixels)) {
        return bitmapFile;
    }
    size_t imageSize = 0;
    if (!BmpTool::SafeMath::multiply(total_pixels, static_cast<size_t>(4), imageSize) || 
        imageSize > BmpTool::SafeMath::MAX_SAFE_IMAGE_BYTES) {
        return bitmapFile;
    }

    // Populate BITMAPINFOHEADER
    bitmapFile.bitmapInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapFile.bitmapInfoHeader.biWidth = static_cast<int32_t>(imageMatrix.cols());
    bitmapFile.bitmapInfoHeader.biHeight = static_cast<int32_t>(imageMatrix.rows());
    bitmapFile.bitmapInfoHeader.biPlanes = 1;
    bitmapFile.bitmapInfoHeader.biBitCount = 32;
    bitmapFile.bitmapInfoHeader.biCompression = BI_RGB;
    bitmapFile.bitmapInfoHeader.biSizeImage = static_cast<uint32_t>(imageSize);

    // Populate BITMAPFILEHEADER
    size_t offsetSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    size_t fileSize = 0;
    if (!BmpTool::SafeMath::add(offsetSize, imageSize, fileSize) || fileSize > std::numeric_limits<uint32_t>::max()) {
        return bitmapFile;
    }

    bitmapFile.bitmapFileHeader.bfSize = static_cast<uint32_t>(fileSize);
    bitmapFile.bitmapFileHeader.bfType = 0x4D42;
    bitmapFile.bitmapFileHeader.bfOffBits = static_cast<uint32_t>(offsetSize);
    bitmapFile.bitmapFileHeader.bfReserved1 = 0;
    bitmapFile.bitmapFileHeader.bfReserved2 = 0;

    bitmapFile.bitmapData.resize(imageSize);

    size_t k = 0;
    for (size_t i = 0; i < imageMatrix.rows(); i++)
        for (size_t j = 0; j < imageMatrix.cols(); j++)
        {
            bitmapFile.bitmapData[k] = imageMatrix[i][j].blue;
            bitmapFile.bitmapData[k + 1] = imageMatrix[i][j].green;
            bitmapFile.bitmapData[k + 2] = imageMatrix[i][j].red;
            bitmapFile.bitmapData[k + 3] = imageMatrix[i][j].alpha;
            k += 4;
        }

    bitmapFile.SetValid();
    return bitmapFile;
}

// Converts an image to greyscale.
Bitmap::File GreyscaleImage(Bitmap::File bitmapFile)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (int i = 0; i < imageMatrix.rows(); ++i)
        for (int j = 0; j < imageMatrix.cols(); ++j)
            imageMatrix[i][j] = GreyScalePixel(imageMatrix[i][j]);
    return CreateBitmapFromMatrix(imageMatrix);
}

// Shrinks an image by an integer scale factor using averaging.
Bitmap::File ShrinkImage(Bitmap::File bitmapFile, int scaleFactor)
{
    if (scaleFactor <= 0) return bitmapFile; // Or handle error appropriately
    scaleFactor = BmpTool::SafeMath::clamp(scaleFactor, 1, BmpTool::SafeMath::MAX_SAFE_SCALE_FACTOR);

    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    // Calculate dimensions of the new, shrunken matrix.
    int newRows = imageMatrix.rows() / scaleFactor;
    int newCols = imageMatrix.cols() / scaleFactor;
    if (newRows == 0 || newCols == 0) return bitmapFile; // Cannot shrink to zero size

    Matrix::Matrix<Pixel> shrunkenMatrix(newRows, newCols);

    for (size_t i = 0; i < static_cast<size_t>(shrunkenMatrix.rows()); i++)
        for (size_t j = 0; j < static_cast<size_t>(shrunkenMatrix.cols()); j++)
        {
            unsigned int averageRed = 0;
            unsigned int averageGreen = 0;
            unsigned int averageBlue = 0;
            unsigned int averageAlpha = 0;
            int numPixels = 0; // Count of pixels in the block for averaging.

            // Iterate over the block of pixels in the original image that corresponds to the current pixel in the shrunken image.
            for (size_t k = 0; (k < static_cast<size_t>(scaleFactor)) && ((k + (i * static_cast<size_t>(scaleFactor))) < static_cast<size_t>(imageMatrix.rows())); k++)
                for (size_t l = 0; (l < static_cast<size_t>(scaleFactor)) && ((l + (j * static_cast<size_t>(scaleFactor))) < static_cast<size_t>(imageMatrix.cols())); l++)
                {
                    const auto& px = imageMatrix[k + (i * static_cast<size_t>(scaleFactor))][l + (j * static_cast<size_t>(scaleFactor))];
                    averageRed += px.red;
                    averageGreen += px.green;
                    averageBlue += px.blue;
                    averageAlpha += px.alpha;
                    numPixels++;
                }

            if (numPixels > 0) {
                shrunkenMatrix[i][j].red = (BYTE)(averageRed / numPixels);
                shrunkenMatrix[i][j].green = (BYTE)(averageGreen / numPixels);
                shrunkenMatrix[i][j].blue = (BYTE)(averageBlue / numPixels);
                shrunkenMatrix[i][j].alpha = (BYTE)(averageAlpha / numPixels);
            }
            // Else, if numPixels is 0 (should not happen if newRows/newCols > 0), pixel remains default (likely black).
        }
    bitmapFile = CreateBitmapFromMatrix(shrunkenMatrix);
    return bitmapFile;
}

// Rotates the image 90 degrees counter-clockwise.
Bitmap::File RotateImageCounterClockwise(Bitmap::File bitmapFile)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    Matrix::Matrix<Pixel> rotatedMatrix(imageMatrix.cols(), imageMatrix.rows());
    for (int i = 0; i < imageMatrix.rows(); i++)
        for (int j = 0; j < imageMatrix.cols(); j++)
            rotatedMatrix[j][rotatedMatrix.cols() - i - 1] = imageMatrix[i][j];
    return CreateBitmapFromMatrix(rotatedMatrix);
}

// Rotates the image 90 degrees clockwise.
Bitmap::File RotateImageClockwise(Bitmap::File bitmapFile)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    Matrix::Matrix<Pixel> rotatedMatrix(imageMatrix.cols(), imageMatrix.rows());
    for (int i = 0; i < imageMatrix.rows(); i++)
        for (int j = 0; j < imageMatrix.cols(); j++)
            rotatedMatrix[rotatedMatrix.rows() - j - 1][i] = imageMatrix[i][j];
    return CreateBitmapFromMatrix(rotatedMatrix);
}

// Mirrors the image horizontally (left to right).
Bitmap::File MirrorImage(Bitmap::File bitmapFile)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    Matrix::Matrix<Pixel> mirroredMatrix(imageMatrix.rows(), imageMatrix.cols());
    for (int i = 0; i < imageMatrix.rows(); i++)
        for (int j = 0; j < imageMatrix.cols(); j++)
            mirroredMatrix[i][mirroredMatrix.cols() - j - 1] = imageMatrix[i][j];
    return CreateBitmapFromMatrix(mirroredMatrix);
}

// Flips the image vertically (top to bottom).
Bitmap::File FlipImage(Bitmap::File bitmapFile)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    Matrix::Matrix<Pixel> flippedMatrix(imageMatrix.rows(), imageMatrix.cols());
    for (int i = 0; i < imageMatrix.rows(); i++)
        for (int j = 0; j < imageMatrix.cols(); j++)
            flippedMatrix[flippedMatrix.rows() - i - 1][j] = imageMatrix[i][j];
    return CreateBitmapFromMatrix(flippedMatrix);
}

// Changes the overall brightness of the image.
Bitmap::File ChangeImageBrightness(Bitmap::File bitmapFile, float brightness)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (int i = 0; i < imageMatrix.rows(); ++i)
        for (int j = 0; j < imageMatrix.cols(); ++j)
            imageMatrix[i][j] = ChangePixelBrightness(imageMatrix[i][j], brightness);
    return CreateBitmapFromMatrix(imageMatrix);
}

// Changes the overall saturation of the image.
Bitmap::File ChangeImageSaturation(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (int i = 0; i < imageMatrix.rows(); ++i)
        for (int j = 0; j < imageMatrix.cols(); ++j)
            imageMatrix[i][j] = ChangePixelSaturation(imageMatrix[i][j], saturation);
    return CreateBitmapFromMatrix(imageMatrix);
}

// Changes the overall contrast of the image.
Bitmap::File ChangeImageContrast(Bitmap::File bitmapFile, float contrast)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (int i = 0; i < imageMatrix.rows(); ++i)
        for (int j = 0; j < imageMatrix.cols(); ++j)
            imageMatrix[i][j] = ChangePixelContrast(imageMatrix[i][j], contrast);
    return CreateBitmapFromMatrix(imageMatrix);
}

// Changes the saturation of the blue channel of a single pixel.
// Only applies if blue is the dominant or co-dominant color.
Pixel ChangePixelSaturationBlue(Pixel pixel, float saturation)
{
    if (!std::isfinite(saturation) || saturation < 0.0f) {
        return pixel;
    }
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    // Check if blue is a dominant component to avoid desaturating other colors.
    if ((pixel.blue >= pixel.red) && (pixel.blue >= pixel.green)) // Simplified condition, original was (pixel.blue >= pixel.red) && (pixel.blue >= pixel.red)
    {
        int new_blue = (int)((pixel.blue - average) * saturation + average);
        pixel.blue = (BYTE)std::max(0, std::min(255, new_blue)); // Clamp to 0-255
    }
    // Alpha remains unchanged.
    return pixel;
}

// Changes the saturation of the green channel of a single pixel.
// Only applies if green is the dominant or co-dominant color.
Pixel ChangePixelSaturationGreen(Pixel pixel, float saturation)
{
    if (!std::isfinite(saturation) || saturation < 0.0f) {
        return pixel;
    }
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    if ((pixel.green >= pixel.red) && (pixel.green >= pixel.blue))
    {
        int new_green = (int)((pixel.green - average) * saturation + average);
        pixel.green = (BYTE)std::max(0, std::min(255, new_green)); // Clamp to 0-255
    }
    // Alpha remains unchanged.
    return pixel;
}

// Changes the saturation of the red channel of a single pixel.
// Only applies if red is the dominant or co-dominant color.
Pixel ChangePixelSaturationRed(Pixel pixel, float saturation)
{
    if (!std::isfinite(saturation) || saturation < 0.0f) {
        return pixel;
    }
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    if ((pixel.red >= pixel.blue) && (pixel.red >= pixel.green))
    {
        int new_red = (int)((pixel.red - average) * saturation + average);
        pixel.red = (BYTE)std::max(0, std::min(255, new_red)); // Clamp to 0-255
    }
    // Alpha remains unchanged.
    return pixel;
}

// Changes the saturation of the magenta component (red and blue channels) of a single pixel.
Pixel ChangePixelSaturationMagenta(Pixel pixel, float saturation)
{
    if (!std::isfinite(saturation) || saturation < 0.0f) {
        return pixel;
    }
    int average = (pixel.red + pixel.blue + pixel.green) / 3;
    int magenta_component = std::min(pixel.red, pixel.blue); // The "amount" of magenta.
    int redOffset = pixel.red - magenta_component;   // How much "more red" than magenta.
    int blueOffset = pixel.blue - magenta_component; // How much "more blue" than magenta.

    if (magenta_component >= average) // Only saturate if magenta is more intense than average.
    {
        magenta_component = (int)((magenta_component - average) * saturation + average);
    }
    // Reconstruct and clamp.
    pixel.red = (BYTE)std::max(0, std::min(255, redOffset + magenta_component));
    pixel.blue = (BYTE)std::max(0, std::min(255, blueOffset + magenta_component));
    // Green and Alpha remain unchanged.
    return pixel;
}

// Changes the saturation of the yellow component (red and green channels) of a single pixel.
Pixel ChangePixelSaturationYellow(Pixel pixel, float saturation)
{
    if (!std::isfinite(saturation) || saturation < 0.0f) {
        return pixel;
    }
    int average = (pixel.red + pixel.blue + pixel.green) / 3;
    int yellow_component = std::min(pixel.red, pixel.green); // The "amount" of yellow.
    int redOffset = pixel.red - yellow_component;    // How much "more red" than yellow.
    int greenOffset = pixel.green - yellow_component; // How much "more green" than yellow.

    if (yellow_component >= average) // Only saturate if yellow is more intense than average.
    {
        yellow_component = (int)((yellow_component - average) * saturation + average);
    }
    // Reconstruct and clamp.
    pixel.red = (BYTE)std::max(0, std::min(255, redOffset + yellow_component));
    pixel.green = (BYTE)std::max(0, std::min(255, greenOffset + yellow_component));
    // Blue and Alpha remain unchanged.
    return pixel;
}

// Changes the saturation of the cyan component (green and blue channels) of a single pixel.
Pixel ChangePixelSaturationCyan(Pixel pixel, float saturation)
{
    if (!std::isfinite(saturation) || saturation < 0.0f) {
        return pixel;
    }
    int average = (pixel.red + pixel.blue + pixel.green) / 3;
    int cyan_component = std::min(pixel.blue, pixel.green); // The "amount" of cyan.
    int greenOffset = pixel.green - cyan_component; // How much "more green" than cyan.
    int blueOffset = pixel.blue - cyan_component;   // How much "more blue" than cyan.

    if (cyan_component >= average) // Only saturate if cyan is more intense than average.
    {
        cyan_component = (int)((cyan_component - average) * saturation + average);
    }
    // Reconstruct and clamp.
    pixel.green = (BYTE)std::max(0, std::min(255, greenOffset + cyan_component));
    pixel.blue = (BYTE)std::max(0, std::min(255, blueOffset + cyan_component));
    // Red and Alpha remain unchanged.
    return pixel;
}

// Changes the saturation of the blue channel in the image.
Bitmap::File ChangeImageSaturationBlue(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (int i = 0; i < imageMatrix.rows(); ++i)
        for (int j = 0; j < imageMatrix.cols(); ++j)
            imageMatrix[i][j] = ChangePixelSaturationBlue(imageMatrix[i][j], saturation);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the saturation of the green channel in the image.
Bitmap::File ChangeImageSaturationGreen(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (int i = 0; i < imageMatrix.rows(); ++i)
        for (int j = 0; j < imageMatrix.cols(); ++j)
            imageMatrix[i][j] = ChangePixelSaturationGreen(imageMatrix[i][j], saturation);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the saturation of the red channel in the image.
Bitmap::File ChangeImageSaturationRed(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (int i = 0; i < imageMatrix.rows(); ++i)
        for (int j = 0; j < imageMatrix.cols(); ++j)
            imageMatrix[i][j] = ChangePixelSaturationRed(imageMatrix[i][j], saturation);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the saturation of the magenta component in the image.
Bitmap::File ChangeImageSaturationMagenta(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (int i = 0; i < imageMatrix.rows(); ++i)
        for (int j = 0; j < imageMatrix.cols(); ++j)
            imageMatrix[i][j] = ChangePixelSaturationMagenta(imageMatrix[i][j], saturation);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the saturation of the yellow component in the image.
Bitmap::File ChangeImageSaturationYellow(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (int i = 0; i < imageMatrix.rows(); ++i)
        for (int j = 0; j < imageMatrix.cols(); ++j)
            imageMatrix[i][j] = ChangePixelSaturationYellow(imageMatrix[i][j], saturation);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the saturation of the cyan component in the image.
Bitmap::File ChangeImageSaturationCyan(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (int i = 0; i < imageMatrix.rows(); ++i)
        for (int j = 0; j < imageMatrix.cols(); ++j)
            imageMatrix[i][j] = ChangePixelSaturationCyan(imageMatrix[i][j], saturation);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the luminance of the blue channel of a single pixel.
// This function adjusts all color channels if blue is dominant, effectively changing the pixel's overall brightness.
Pixel ChangePixelLuminanceBlue(Pixel pixel, float luminance)
{
    if (!std::isfinite(luminance) || luminance < 0.0f) {
        return pixel;
    }
    if ((pixel.blue >= pixel.red) && (pixel.blue >= pixel.green)) // Only if blue is dominant or co-dominant
    {
        int average = (pixel.red + pixel.green + pixel.blue) / 3;
        int newAverage = (int)(average * luminance);

        int new_red_val = (pixel.red - average) + newAverage;
        pixel.red = (BYTE)std::max(0, std::min(255, new_red_val));

        int new_green_val = (pixel.green - average) + newAverage;
        pixel.green = (BYTE)std::max(0, std::min(255, new_green_val));

        int new_blue_val = (pixel.blue - average) + newAverage;
        pixel.blue = (BYTE)std::max(0, std::min(255, new_blue_val));
    }
    // Alpha remains unchanged.
    return pixel;
}

// Changes the luminance of the green channel of a single pixel.
// Adjusts all color channels if green is dominant.
Pixel ChangePixelLuminanceGreen(Pixel pixel, float luminance)
{
    if (!std::isfinite(luminance) || luminance < 0.0f) {
        return pixel;
    }
    if ((pixel.green >= pixel.red) && (pixel.green >= pixel.blue)) // Only if green is dominant or co-dominant
    {
        int average = (pixel.red + pixel.green + pixel.blue) / 3;
        int newAverage = (int)(average * luminance);

        int new_red_val = (pixel.red - average) + newAverage;
        pixel.red = (BYTE)std::max(0, std::min(255, new_red_val));

        int new_green_val = (pixel.green - average) + newAverage;
        pixel.green = (BYTE)std::max(0, std::min(255, new_green_val));

        int new_blue_val = (pixel.blue - average) + newAverage;
        pixel.blue = (BYTE)std::max(0, std::min(255, new_blue_val));
    }
    // Alpha remains unchanged.
    return pixel;
}

// Changes the luminance of the red channel of a single pixel.
// Adjusts all color channels if red is dominant.
Pixel ChangePixelLuminanceRed(Pixel pixel, float luminance)
{
    if (!std::isfinite(luminance) || luminance < 0.0f) {
        return pixel;
    }
    if ((pixel.red >= pixel.green) && (pixel.red >= pixel.blue)) // Only if red is dominant or co-dominant
    {
        int average = (pixel.red + pixel.green + pixel.blue) / 3;
        int newAverage = (int)(average * luminance);

        int new_red_val = (pixel.red - average) + newAverage;
        pixel.red = (BYTE)std::max(0, std::min(255, new_red_val));

        int new_green_val = (pixel.green - average) + newAverage;
        pixel.green = (BYTE)std::max(0, std::min(255, new_green_val));

        int new_blue_val = (pixel.blue - average) + newAverage;
        pixel.blue = (BYTE)std::max(0, std::min(255, new_blue_val));
    }
    // Alpha remains unchanged.
    return pixel;
}

// Changes the luminance of the magenta component (red and blue channels) of a single pixel.
// Adjusts all color channels if magenta (min(R,B)) is more intense than the pixel's average intensity.
Pixel ChangePixelLuminanceMagenta(Pixel pixel, float luminance)
{
    if (!std::isfinite(luminance) || luminance < 0.0f) {
        return pixel;
    }
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    int magenta_component = std::min(pixel.red, pixel.blue);
    if (magenta_component > average) // Only if magenta component is above average intensity
    {
        int newAverage = (int)(average * luminance);

        int new_red_val = (pixel.red - average) + newAverage;
        pixel.red = (BYTE)std::max(0, std::min(255, new_red_val));

        int new_green_val = (pixel.green - average) + newAverage;
        pixel.green = (BYTE)std::max(0, std::min(255, new_green_val));

        int new_blue_val = (pixel.blue - average) + newAverage;
        pixel.blue = (BYTE)std::max(0, std::min(255, new_blue_val));
    }
    // Alpha remains unchanged.
    return pixel;
}

// Changes the luminance of the yellow component (red and green channels) of a single pixel.
// Adjusts all color channels if yellow (min(R,G)) is more intense than the pixel's average intensity.
Pixel ChangePixelLuminanceYellow(Pixel pixel, float luminance)
{
    if (!std::isfinite(luminance) || luminance < 0.0f) {
        return pixel;
    }
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    int yellow_component = std::min(pixel.red, pixel.green);
    if (yellow_component > average) // Only if yellow component is above average intensity
    {
        int newAverage = (int)(average * luminance);

        int new_red_val = (pixel.red - average) + newAverage;
        pixel.red = (BYTE)std::max(0, std::min(255, new_red_val));

        int new_green_val = (pixel.green - average) + newAverage;
        pixel.green = (BYTE)std::max(0, std::min(255, new_green_val));

        int new_blue_val = (pixel.blue - average) + newAverage;
        pixel.blue = (BYTE)std::max(0, std::min(255, new_blue_val));
    }
    // Alpha remains unchanged.
    return pixel;
}

// Changes the luminance of the cyan component (green and blue channels) of a single pixel.
// Adjusts all color channels if cyan (min(G,B)) is more intense than the pixel's average intensity.
Pixel ChangePixelLuminanceCyan(Pixel pixel, float luminance)
{
    if (!std::isfinite(luminance) || luminance < 0.0f) {
        return pixel;
    }
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    int cyan_component = std::min(pixel.green, pixel.blue);
    if (cyan_component > average) // Only if cyan component is above average intensity
    {
        int newAverage = (int)(average * luminance);

        int new_red_val = (pixel.red - average) + newAverage;
        pixel.red = (BYTE)std::max(0, std::min(255, new_red_val));

        int new_green_val = (pixel.green - average) + newAverage;
        pixel.green = (BYTE)std::max(0, std::min(255, new_green_val));

        int new_blue_val = (pixel.blue - average) + newAverage;
        pixel.blue = (BYTE)std::max(0, std::min(255, new_blue_val));
    }
    // Alpha remains unchanged.
    return pixel;
}

// Changes the luminance of the blue channel in the image.
// Note: The 'luminance' parameter name in the function signature here matches the .h file.
// The actual pixel manipulation is done by ChangePixelLuminanceBlue which takes 'luminance'.
Bitmap::File ChangeImageLuminanceBlue(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (int i = 0; i < imageMatrix.rows(); ++i)
        for (int j = 0; j < imageMatrix.cols(); ++j)
            imageMatrix[i][j] = ChangePixelLuminanceBlue(imageMatrix[i][j], luminance);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the luminance of the green channel in the image.
Bitmap::File ChangeImageLuminanceGreen(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (int i = 0; i < imageMatrix.rows(); ++i)
        for (int j = 0; j < imageMatrix.cols(); ++j)
            imageMatrix[i][j] = ChangePixelLuminanceGreen(imageMatrix[i][j], luminance);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the luminance of the red channel in the image.
Bitmap::File ChangeImageLuminanceRed(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (int i = 0; i < imageMatrix.rows(); ++i)
        for (int j = 0; j < imageMatrix.cols(); ++j)
            imageMatrix[i][j] = ChangePixelLuminanceRed(imageMatrix[i][j], luminance);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the luminance of the magenta component in the image.
Bitmap::File ChangeImageLuminanceMagenta(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (int i = 0; i < imageMatrix.rows(); ++i)
        for (int j = 0; j < imageMatrix.cols(); ++j)
            imageMatrix[i][j] = ChangePixelLuminanceMagenta(imageMatrix[i][j], luminance);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the luminance of the yellow component in the image.
Bitmap::File ChangeImageLuminanceYellow(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (int i = 0; i < imageMatrix.rows(); ++i)
        for (int j = 0; j < imageMatrix.cols(); ++j)
            imageMatrix[i][j] = ChangePixelLuminanceYellow(imageMatrix[i][j], luminance);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the luminance of the cyan component in the image.
Bitmap::File ChangeImageLuminanceCyan(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (int i = 0; i < imageMatrix.rows(); ++i)
        for (int j = 0; j < imageMatrix.cols(); ++j)
            imageMatrix[i][j] = ChangePixelLuminanceCyan(imageMatrix[i][j], luminance);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Converts a pixel to greyscale by averaging its RGB components.
Pixel GreyScalePixel(Pixel pixel) {
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    pixel.red = average;
    pixel.green = average;
    pixel.blue = average;
    return pixel;
}

// Changes the brightness of a pixel. The brightness is adjusted by scaling the pixel's RGB values around their average.
Pixel ChangePixelBrightness(Pixel pixel, float brightness) {
    if (!std::isfinite(brightness) || brightness < 0.0f) {
        return pixel;
    }
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    int newAverage = static_cast<int>(average * brightness);
    int new_red = (pixel.red - average) + newAverage;
    int new_green = (pixel.green - average) + newAverage;
    int new_blue = (pixel.blue - average) + newAverage;
    pixel.red = std::clamp(new_red, 0, 255);
    pixel.green = std::clamp(new_green, 0, 255);
    pixel.blue = std::clamp(new_blue, 0, 255);
    return pixel;
}

// Changes the saturation of a pixel. Increases or decreases the pixel's RGB values based on their distance from the average.
Pixel ChangePixelSaturation(Pixel pixel, float saturation) {
    if (!std::isfinite(saturation) || saturation < 0.0f) {
        return pixel;
    }
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    if (pixel.red > average)
        pixel.red = std::clamp(static_cast<int>((pixel.red - average) * saturation + average), 0, 255);
    if (pixel.green > average)
        pixel.green = std::clamp(static_cast<int>((pixel.green - average) * saturation + average), 0, 255);
    if (pixel.blue > average)
        pixel.blue = std::clamp(static_cast<int>((pixel.blue - average) * saturation + average), 0, 255);
    return pixel;
}

// Changes the contrast of a pixel. Stretches or compresses the pixel's RGB values around the midpoint (128).
Pixel ChangePixelContrast(Pixel pixel, float contrast) {
    if (!std::isfinite(contrast) || contrast < 0.0f) {
        return pixel;
    }
    pixel.red = std::clamp(static_cast<int>(128 + (pixel.red - 128) * contrast), 0, 255);
    pixel.green = std::clamp(static_cast<int>(128 + (pixel.green - 128) * contrast), 0, 255);
    pixel.blue = std::clamp(static_cast<int>(128 + (pixel.blue - 128) * contrast), 0, 255);
    return pixel;
}

Pixel ChangePixelContrastRed(Pixel pixel, float contrast) {
    if (!std::isfinite(contrast) || contrast < 0.0f) {
        return pixel;
    }
    pixel.red = std::clamp(static_cast<int>(128 + (pixel.red - 128) * contrast), 0, 255);
    return pixel;
}

Pixel ChangePixelContrastGreen(Pixel pixel, float contrast) {
    if (!std::isfinite(contrast) || contrast < 0.0f) {
        return pixel;
    }
    pixel.green = std::clamp(static_cast<int>(128 + (pixel.green - 128) * contrast), 0, 255);
    return pixel;
}

Pixel ChangePixelContrastBlue(Pixel pixel, float contrast) {
    if (!std::isfinite(contrast) || contrast < 0.0f) {
        return pixel;
    }
    pixel.blue = std::clamp(static_cast<int>(128 + (pixel.blue - 128) * contrast), 0, 255);
    return pixel;
}

Pixel ChangePixelContrastMagenta(Pixel pixel, float contrast) {
    if (!std::isfinite(contrast) || contrast < 0.0f) {
        return pixel;
    }
    pixel.red = std::clamp(static_cast<int>(128 + (pixel.red - 128) * contrast), 0, 255);
    pixel.blue = std::clamp(static_cast<int>(128 + (pixel.blue - 128) * contrast), 0, 255);
    return pixel;
}

Pixel ChangePixelContrastYellow(Pixel pixel, float contrast) {
    if (!std::isfinite(contrast) || contrast < 0.0f) {
        return pixel;
    }
    pixel.red = std::clamp(static_cast<int>(128 + (pixel.red - 128) * contrast), 0, 255);
    pixel.green = std::clamp(static_cast<int>(128 + (pixel.green - 128) * contrast), 0, 255);
    return pixel;
}

Pixel ChangePixelContrastCyan(Pixel pixel, float contrast) {
    if (!std::isfinite(contrast) || contrast < 0.0f) {
        return pixel;
    }
    pixel.green = std::clamp(static_cast<int>(128 + (pixel.green - 128) * contrast), 0, 255);
    pixel.blue = std::clamp(static_cast<int>(128 + (pixel.blue - 128) * contrast), 0, 255);
    return pixel;
}