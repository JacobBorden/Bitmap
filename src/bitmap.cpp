#include "bitmap.h"
#include <iostream>  // For standard I/O (though not explicitly used in this file's current state).
#include <algorithm> // For std::min and std::max, used in ApplyBoxBlur and color adjustments.
#include <vector>    // For std::vector, used by Matrix class and underlying bitmap data.

// Captures a screenshot of a specified window and returns it as a Bitmap::File object.
// This function uses Windows API calls to interact with window handles and device contexts.
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
        bitmapFile.bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapFile.bitmapInfo.bmiHeader.biWidth = deviceContextBitmap.bmWidth;
        bitmapFile.bitmapInfo.bmiHeader.biHeight = deviceContextBitmap.bmHeight; // Positive height for bottom-up DIB.
        bitmapFile.bitmapInfo.bmiHeader.biPlanes = deviceContextBitmap.bmPlanes; // Usually 1.
        bitmapFile.bitmapInfo.bmiHeader.biBitCount = deviceContextBitmap.bmBitsPixel; // Bits per pixel (e.g., 24 or 32).
        bitmapFile.bitmapInfo.bmiHeader.biCompression = BI_RGB; // Uncompressed RGB.
        // Calculate image size in bytes. For BI_RGB, this can be 0 if biHeight is positive.
        // However, explicitly calculating it is safer for raw data access.
        int imageSize = deviceContextBitmap.bmWidth * deviceContextBitmap.bmHeight * (deviceContextBitmap.bmBitsPixel / 8);
        bitmapFile.bitmapInfo.bmiHeader.biSizeImage = imageSize; // Total size of the image data.
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
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = InvertPixelColor(pixels);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
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
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ApplySepiaToPixel(pixels);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
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
    for (int r = 0; r < originalMatrix.rows(); ++r) // r for row
    {
        for (int c = 0; c < originalMatrix.cols(); ++c) // c for column
        {
            unsigned int sumRed = 0, sumGreen = 0, sumBlue = 0, sumAlpha = 0;
            int count = 0; // Number of pixels included in the blur box.

            // Iterate over the box defined by blurRadius around the current pixel (r, c).
            // std::max and std::min are used to handle boundary conditions, ensuring we don't go out of bounds.
            for (int i = std::max(0, r - blurRadius); i <= std::min(originalMatrix.rows() - 1, r + blurRadius); ++i)
            {
                for (int j = std::max(0, c - blurRadius); j <= std::min(originalMatrix.cols() - 1, c + blurRadius); ++j)
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
    Matrix::Matrix<Pixel> imageMatrix(bitmapFile.bitmapInfo.bmiHeader.biHeight, bitmapFile.bitmapInfo.bmiHeader.biWidth);

    if (bitmapFile.bitmapInfo.bmiHeader.biBitCount == 32) // For 32-bit bitmaps (BGRA)
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
    else if (bitmapFile.bitmapInfo.bmiHeader.biBitCount == 24) // For 24-bit bitmaps (BGR)
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
Bitmap::File CreateBitmapFromMatrix(Matrix::Matrix<Pixel> imageMatrix)
{
    Bitmap::File bitmapFile;

    // Populate BITMAPINFOHEADER
    bitmapFile.bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapFile.bitmapInfo.bmiHeader.biWidth = imageMatrix.cols();
    bitmapFile.bitmapInfo.bmiHeader.biHeight = imageMatrix.rows(); // Positive height for bottom-up DIB.
    bitmapFile.bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapFile.bitmapInfo.bmiHeader.biBitCount = 32; // Outputting as 32-bit BGRA.
    bitmapFile.bitmapInfo.bmiHeader.biCompression = BI_RGB; // Uncompressed.
    // Calculate image size in bytes for a 32-bit image.
    int imageSize = imageMatrix.size() * (32 / 8); // imageMatrix.size() is rows * cols.
    bitmapFile.bitmapInfo.bmiHeader.biSizeImage = imageSize;
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
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = GreyScalePixel(pixels);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
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
    bitmapFile = CreateBitmapFromMatrix(rotatedMatrix);
    return bitmapFile;
}

// Rotates the image 90 degrees clockwise.
Bitmap::File RotateImageClockwise(Bitmap::File bitmapFile)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    Matrix::Matrix<Pixel> rotatedMatrix(imageMatrix.cols(), imageMatrix.rows());
    for (int i = 0; i < imageMatrix.rows(); i++)
        for (int j = 0; j < imageMatrix.cols(); j++)
            rotatedMatrix[rotatedMatrix.rows() - j - 1][i] = imageMatrix[i][j];
    bitmapFile = CreateBitmapFromMatrix(rotatedMatrix);
    return bitmapFile;
}

// Mirrors the image horizontally (left to right).
Bitmap::File MirrorImage(Bitmap::File bitmapFile)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    Matrix::Matrix<Pixel> mirroredMatrix(imageMatrix.rows(), imageMatrix.cols());
    for (int i = 0; i < imageMatrix.rows(); i++)
        for (int j = 0; j < imageMatrix.cols(); j++)
            mirroredMatrix[i][mirroredMatrix.cols() - j - 1] = imageMatrix[i][j];
    bitmapFile = CreateBitmapFromMatrix(mirroredMatrix);
    return bitmapFile;
}

// Flips the image vertically (top to bottom).
Bitmap::File FlipImage(Bitmap::File bitmapFile)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    Matrix::Matrix<Pixel> flippedMatrix(imageMatrix.rows(), imageMatrix.cols());
    for (int i = 0; i < imageMatrix.rows(); i++)
        for (int j = 0; j < imageMatrix.cols(); j++)
            flippedMatrix[flippedMatrix.rows() - i - 1][j] = imageMatrix[i][j];
    bitmapFile = CreateBitmapFromMatrix(flippedMatrix);
    return bitmapFile;
}

// Changes the overall brightness of the image.
Bitmap::File ChangeImageBrightness(Bitmap::File bitmapFile, float brightness)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelBrightness(pixels, brightness);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

// Changes the overall saturation of the image.
Bitmap::File ChangeImageSaturation(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelSaturation(pixels, saturation);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Converts a single pixel to its greyscale equivalent.
// Greyscale is calculated by averaging the red, green, and blue components.
Pixel GreyScalePixel(Pixel pixel)
{
    int average = (pixel.red + pixel.blue + pixel.green) / 3;
    pixel.blue = average;
    pixel.red = average;
    pixel.green = average;
    return pixel;
}

// Changes the brightness of a single pixel.
// It calculates the average intensity and then scales each color component relative to this average.
Pixel ChangePixelBrightness(Pixel pixel, float brightness) // Parameter name was 'brightnessl' in header, using 'brightness' here.
{
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    int newAverage = (int)(average * brightness);

    // Calculate new red value, clamping to 0-255
    int new_red_val = (pixel.red - average) + newAverage;
    if (new_red_val < 0) pixel.red = 0;
    else if (new_red_val > 255) pixel.red = 255;
    else pixel.red = (BYTE)new_red_val;

    // Calculate new green value, clamping to 0-255
    int new_green_val = (pixel.green - average) + newAverage;
    if (new_green_val < 0) pixel.green = 0;
    else if (new_green_val > 255) pixel.green = 255;
    else pixel.green = (BYTE)new_green_val;

    // Calculate new blue value, clamping to 0-255
    int new_blue_val = (pixel.blue - average) + newAverage;
    if (new_blue_val < 0) pixel.blue = 0;
    else if (new_blue_val > 255) pixel.blue = 255;
    else pixel.blue = (BYTE)new_blue_val;
    // Alpha remains unchanged.
    return pixel;
}

// Changes the saturation of a single pixel.
// It adjusts how far each color component is from the average intensity (greyscale).
Pixel ChangePixelSaturation(Pixel pixel, float saturation)
{
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    if (pixel.red > average)
    {
        if ((((pixel.red - average) * saturation + average) <= 255) && (((pixel.red - average) * saturation + average) >= 0))
            pixel.red = (BYTE)((pixel.red - average) * saturation + average);
        else if ((((pixel.red - average) * saturation + average) > 255))
            pixel.red = 255;
        else
            pixel.red = 0;
    }
    if (pixel.green > average)
    {
        if ((((pixel.green - average) * saturation + average) <= 255) && (((pixel.green - average) * saturation + average) >= 0))
            pixel.green = (BYTE)((pixel.green - average) * saturation + average);
        else if ((((pixel.green - average) * saturation + average) > 255))
            pixel.green = 255;
        else
            pixel.green = 0;
    }

    if (pixel.blue > average)
    {
        if ((((pixel.blue - average) * saturation + average) <= 255) && (((pixel.blue - average) * saturation + average) >= 0))
            pixel.blue = (BYTE)((pixel.blue - average) * saturation + average);
        else if ((((pixel.blue - average) * saturation + average) > 255))
            pixel.blue = 255;
        else
            pixel.blue = 0;
    }
    return pixel;
}

// Changes the overall contrast of the image.
Bitmap::File ChangeImageContrast(Bitmap::File bitmapFile, float contrast)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelContrast(pixels, contrast);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

// Changes the contrast of a single pixel.
// This is done by scaling the difference of each color component from a mid-point (128).
Pixel ChangePixelContrast(Pixel pixel, float contrast)
{
    // Adjust red component
    int new_red = (int)(128 + (pixel.red - 128) * contrast);
    pixel.red = (BYTE)std::max(0, std::min(255, new_red)); // Clamp to 0-255

    // Adjust green component
    int new_green = (int)(128 + (pixel.green - 128) * contrast);
    pixel.green = (BYTE)std::max(0, std::min(255, new_green)); // Clamp to 0-255

    // Adjust blue component
    int new_blue = (int)(128 + (pixel.blue - 128) * contrast);
    pixel.blue = (BYTE)std::max(0, std::min(255, new_blue)); // Clamp to 0-255
    // Alpha remains unchanged.
    return pixel;
}

// Changes the contrast of the red channel of a single pixel.
Pixel ChangePixelContrastRed(Pixel pixel, float contrast)
{
    int new_red = (int)(128 + (pixel.red - 128) * contrast);
    pixel.red = (BYTE)std::max(0, std::min(255, new_red)); // Clamp to 0-255
    // Other channels and alpha remain unchanged.
    return pixel;
}

// Changes the contrast of the green channel of a single pixel.
Pixel ChangePixelContrastGreen(Pixel pixel, float contrast)
{
    int new_green = (int)(128 + (pixel.green - 128) * contrast);
    pixel.green = (BYTE)std::max(0, std::min(255, new_green)); // Clamp to 0-255
    // Other channels and alpha remain unchanged.
    return pixel;
}

// Changes the contrast of the blue channel of a single pixel.
Pixel ChangePixelContrastBlue(Pixel pixel, float contrast)
{
    int new_blue = (int)(128 + (pixel.blue - 128) * contrast);
    pixel.blue = (BYTE)std::max(0, std::min(255, new_blue)); // Clamp to 0-255
    // Other channels and alpha remain unchanged.
    return pixel;
}

// Applies contrast adjustment to the red and blue channels of a pixel (Magenta).
// Contrast is applied directly to the constituent primary color channels.
Pixel ChangePixelContrastMagenta(Pixel pixel, float contrast)
{
    // Adjust red component
    int new_red = (int)(128 + (pixel.red - 128) * contrast);
    pixel.red = (BYTE)std::max(0, std::min(255, new_red)); // Clamp to 0-255

    // Adjust blue component
    int new_blue = (int)(128 + (pixel.blue - 128) * contrast);
    pixel.blue = (BYTE)std::max(0, std::min(255, new_blue)); // Clamp to 0-255

    // Green channel remains unchanged for magenta contrast
    return pixel;
}

// Applies contrast adjustment to the red and green channels of a pixel (Yellow).
// Contrast is applied directly to the constituent primary color channels.
Pixel ChangePixelContrastYellow(Pixel pixel, float contrast)
{
    // Adjust red component
    int new_red = (int)(128 + (pixel.red - 128) * contrast);
    pixel.red = (BYTE)std::max(0, std::min(255, new_red)); // Clamp to 0-255

    // Adjust green component
    int new_green = (int)(128 + (pixel.green - 128) * contrast);
    pixel.green = (BYTE)std::max(0, std::min(255, new_green)); // Clamp to 0-255

    // Blue channel remains unchanged for yellow contrast
    return pixel;
}

// Applies contrast adjustment to the green and blue channels of a pixel (Cyan).
// Contrast is applied directly to the constituent primary color channels.
Pixel ChangePixelContrastCyan(Pixel pixel, float contrast)
{
    // Adjust green component
    int new_green = (int)(128 + (pixel.green - 128) * contrast);
    pixel.green = (BYTE)std::max(0, std::min(255, new_green)); // Clamp to 0-255

    // Adjust blue component
    int new_blue = (int)(128 + (pixel.blue - 128) * contrast);
    pixel.blue = (BYTE)std::max(0, std::min(255, new_blue)); // Clamp to 0-255

    // Red channel remains unchanged for cyan contrast
    return pixel;
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
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelSaturationBlue(pixels, saturation);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the saturation of the green channel in the image.
Bitmap::File ChangeImageSaturationGreen(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelSaturationGreen(pixels, saturation);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the saturation of the red channel in the image.
Bitmap::File ChangeImageSaturationRed(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelSaturationRed(pixels, saturation);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the saturation of the magenta component in the image.
Bitmap::File ChangeImageSaturationMagenta(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelSaturationMagenta(pixels, saturation);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the saturation of the yellow component in the image.
Bitmap::File ChangeImageSaturationYellow(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelSaturationYellow(pixels, saturation);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the saturation of the cyan component in the image.
Bitmap::File ChangeImageSaturationCyan(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelSaturationCyan(pixels, saturation);
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
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelLuminanceBlue(pixels, luminance);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the luminance of the green channel in the image.
Bitmap::File ChangeImageLuminanceGreen(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelLuminanceGreen(pixels, luminance);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the luminance of the red channel in the image.
Bitmap::File ChangeImageLuminanceRed(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelLuminanceRed(pixels, luminance);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the luminance of the magenta component in the image.
Bitmap::File ChangeImageLuminanceMagenta(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelLuminanceMagenta(pixels, luminance);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the luminance of the yellow component in the image.
Bitmap::File ChangeImageLuminanceYellow(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelLuminanceYellow(pixels, luminance);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

// Changes the luminance of the cyan component in the image.
Bitmap::File ChangeImageLuminanceCyan(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelLuminanceCyan(pixels, luminance);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}