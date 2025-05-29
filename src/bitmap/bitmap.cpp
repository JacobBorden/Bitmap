#include "bitmap.h"
#include <iostream>  // For standard I/O (though not explicitly used in this file's current state).
#include <algorithm> // For std::min and std::max, used in ApplyBoxBlur and color adjustments.
#include <vector>    // For std::vector, used by Matrix class and underlying bitmap data.

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

        // Retrieve the actual pixel data from the bitmap.
        // DIB_RGB_COLORS indicates that the bmiColors member of BITMAPINFO is RGB.
        int DIBitsGotSuccessfully = GetDIBits(deviceContextHandle, bitmapHandle, 0, deviceContextBitmap.bmHeight, &bitmapFile.bitmapData[0], &bitmapFile.bitmapInfo, DIB_RGB_COLORS);
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

// Converts a Bitmap::File object (containing raw bitmap data and headers)
// into a Matrix::Matrix<Pixel> for easier pixel manipulation.
Matrix::Matrix<Pixel> CreateMatrixFromBitmap(Bitmap::File bitmapFile)
{
    // Initialize the matrix with dimensions from the bitmap header.
    // Note: Bitmap rows are often stored bottom-up, but matrix access is typically top-down.
    // The loop structure (i from 0 to rows-1) handles this naturally if pixel data is ordered correctly.
    Matrix::Matrix<Pixel> imageMatrix(bitmapFile.bitmapInfoHeader.biHeight, bitmapFile.bitmapInfoHeader.biWidth);


    if (imageMatrix.rows() <= 0 || imageMatrix.cols() <= 0) {
        // Or handle as an error, depending on how Matrix constructor handles non-positive dims.
        // Assuming Matrix constructor ensures rows/cols are positive if biHeight/biWidth were,
        // or if biHeight is negative, it uses abs(biHeight). Let's use imageMatrix dimensions.
        return imageMatrix; // Return empty/default matrix
    }

    unsigned int bpp = bitmapFile.bitmapInfoHeader.biBitCount;
    if (bpp != 24 && bpp != 32) {
        // This function only handles 24 and 32 bpp as per its structure.
        // std::cerr << "CreateMatrixFromBitmap Error: Unsupported bit depth " << bpp << std::endl;
        return imageMatrix; // Return empty/default matrix
    }

    size_t bytes_per_pixel = bpp / 8;
    // Use uint64_t for expected_data_size to prevent overflow during this calculation
    // if imageMatrix.rows() or imageMatrix.cols() are very large.
    uint64_t expected_data_size = static_cast<uint64_t>(imageMatrix.rows()) * imageMatrix.cols() * bytes_per_pixel;

    if (static_cast<uint64_t>(bitmapFile.bitmapData.size()) < expected_data_size) {
        // std::cerr << "CreateMatrixFromBitmap Error: bitmapData.size() " << bitmapFile.bitmapData.size()
        //           << " is less than expected_data_size " << expected_data_size
        //           << " for dimensions " << imageMatrix.rows() << "x" << imageMatrix.cols()
        //           << " at " << bpp << "bpp." << std::endl;
        return imageMatrix; // Return empty/default matrix as data is insufficient
    }

    if (bitmapFile.bitmapInfoHeader.biBitCount == 32) // For 32-bit bitmaps (BGRA)
    {
        int k = 0; // Index for bitmapFile.bitmapData
        for (int i = 0; i < imageMatrix.rows(); i++)
            for (int j = 0; j < imageMatrix.cols(); j++)
            {
                // Pixel data in bitmap files is typically stored in BGR or BGRA order.
                imageMatrix[i][j].blue = bitmapFile.bitmapData[k];
                imageMatrix[i][j].green = bitmapFile.bitmapData[k + 1];
                imageMatrix[i][j].red = bitmapFile.bitmapData[k + 2];
                imageMatrix[i][j].alpha = bitmapFile.bitmapData[k + 3];
                k += 4; // Move to the next pixel (4 bytes)
            }
    }
    else if (bitmapFile.bitmapInfoHeader.biBitCount == 24) // For 24-bit bitmaps (BGR)
    {
        int k = 0; // Index for bitmapFile.bitmapData
        for (int i = 0; i < imageMatrix.rows(); i++)
            for (int j = 0; j < imageMatrix.cols(); j++)
            {
                imageMatrix[i][j].blue = bitmapFile.bitmapData[k];
                imageMatrix[i][j].green = bitmapFile.bitmapData[k + 1];
                imageMatrix[i][j].red = bitmapFile.bitmapData[k + 2];
                imageMatrix[i][j].alpha = 0; // Default alpha to 0 (opaque) for 24-bit images.
                k += 3; // Move to the next pixel (3 bytes)
            }
    }
    // Note: Other bit depths (e.g., 1, 4, 8, 16-bit) would require more complex handling.

    return imageMatrix;
}

// Converts a Matrix::Matrix<Pixel> (representing an image)
// back into a Bitmap::File object (with raw bitmap data and headers).
// Assumes output is always a 32-bit bitmap.
Bitmap::File CreateBitmapFromMatrix(const Matrix::Matrix<Pixel>& imageMatrix)
{
    Bitmap::File bitmapFile;

    // Populate BITMAPINFOHEADER
    bitmapFile.bitmapInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapFile.bitmapInfoHeader.biWidth = imageMatrix.cols();
    bitmapFile.bitmapInfoHeader.biHeight = imageMatrix.rows(); // Positive height for bottom-up DIB.
    bitmapFile.bitmapInfoHeader.biPlanes = 1;
    bitmapFile.bitmapInfoHeader.biBitCount = 32; // Outputting as 32-bit BGRA.
    bitmapFile.bitmapInfoHeader.biCompression = BI_RGB; // Uncompressed.
    // Calculate image size in bytes for a 32-bit image.
    int imageSize = imageMatrix.size() * (32 / 8); // imageMatrix.size() is rows * cols.
    bitmapFile.bitmapInfoHeader.biSizeImage = imageSize;
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

    int k = 0; // Index for bitmapFile.bitmapData
    for (int i = 0; i < imageMatrix.rows(); i++)
        for (int j = 0; j < imageMatrix.cols(); j++)
        {
            // Store pixel data in BGRA order.
            bitmapFile.bitmapData[k] = imageMatrix[i][j].blue;
            bitmapFile.bitmapData[k + 1] = imageMatrix[i][j].green;
            bitmapFile.bitmapData[k + 2] = imageMatrix[i][j].red;
            bitmapFile.bitmapData[k + 3] = imageMatrix[i][j].alpha;
            k += 4; // Move to the next pixel (4 bytes)
        }

    bitmapFile.SetValid(); // Mark the bitmap file as valid.
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

    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    // Calculate dimensions of the new, shrunken matrix.
    int newRows = imageMatrix.rows() / scaleFactor;
    int newCols = imageMatrix.cols() / scaleFactor;
    if (newRows == 0 || newCols == 0) return bitmapFile; // Cannot shrink to zero size

    Matrix::Matrix<Pixel> shrunkenMatrix(newRows, newCols);

    for (int i = 0; i < shrunkenMatrix.rows(); i++)
        for (int j = 0; j < shrunkenMatrix.cols(); j++)
        {
            unsigned int averageRed = 0;
            unsigned int averageGreen = 0;
            unsigned int averageBlue = 0;
            unsigned int averageAlpha = 0;
            int numPixels = 0; // Count of pixels in the block for averaging.

            // Iterate over the block of pixels in the original image that corresponds to the current pixel in the shrunken image.
            for (int k = 0; (k < scaleFactor) && ((k + (i * scaleFactor)) < imageMatrix.rows()); k++)
                for (int l = 0; (l < scaleFactor) && ((l + (j * scaleFactor)) < imageMatrix.cols()); l++)
                {
                    averageRed += imageMatrix[k + (i * scaleFactor)][l + (j * scaleFactor)].red;
                    averageGreen += imageMatrix[k + (i * scaleFactor)][l + (j * scaleFactor)].green;
                    averageBlue += imageMatrix[k + (i * scaleFactor)][l + (j * scaleFactor)].blue;
                    averageAlpha += imageMatrix[k + (i * scaleFactor)][l + (j * scaleFactor)].alpha;
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
    pixel.red = std::clamp(static_cast<int>(128 + (pixel.red - 128) * contrast), 0, 255);
    pixel.green = std::clamp(static_cast<int>(128 + (pixel.green - 128) * contrast), 0, 255);
    pixel.blue = std::clamp(static_cast<int>(128 + (pixel.blue - 128) * contrast), 0, 255);
    return pixel;
}

Pixel ChangePixelContrastRed(Pixel pixel, float contrast) {
    pixel.red = std::clamp(static_cast<int>(128 + (pixel.red - 128) * contrast), 0, 255);
    return pixel;
}

Pixel ChangePixelContrastGreen(Pixel pixel, float contrast) {
    pixel.green = std::clamp(static_cast<int>(128 + (pixel.green - 128) * contrast), 0, 255);
    return pixel;
}

Pixel ChangePixelContrastBlue(Pixel pixel, float contrast) {
    pixel.blue = std::clamp(static_cast<int>(128 + (pixel.blue - 128) * contrast), 0, 255);
    return pixel;
}

Pixel ChangePixelContrastMagenta(Pixel pixel, float contrast) {
    pixel.red = std::clamp(static_cast<int>(128 + (pixel.red - 128) * contrast), 0, 255);
    pixel.blue = std::clamp(static_cast<int>(128 + (pixel.blue - 128) * contrast), 0, 255);
    return pixel;
}

Pixel ChangePixelContrastYellow(Pixel pixel, float contrast) {
    pixel.red = std::clamp(static_cast<int>(128 + (pixel.red - 128) * contrast), 0, 255);
    pixel.green = std::clamp(static_cast<int>(128 + (pixel.green - 128) * contrast), 0, 255);
    return pixel;
}

Pixel ChangePixelContrastCyan(Pixel pixel, float contrast) {
    pixel.green = std::clamp(static_cast<int>(128 + (pixel.green - 128) * contrast), 0, 255);
    pixel.blue = std::clamp(static_cast<int>(128 + (pixel.blue - 128) * contrast), 0, 255);
    return pixel;
}