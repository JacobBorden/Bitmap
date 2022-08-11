#include "bitmap.h"
#include <iostream>

Bitmap::File ScreenShotWindow(HWND windowHandle)
{
    Bitmap::File bitmapFile;
    OpenIcon(windowHandle);
    BringWindowToTop(windowHandle);
    SetActiveWindow(windowHandle);
    HDC deviceContextHandle = GetDC(windowHandle);
    HDC deviceContext = CreateCompatibleDC(deviceContextHandle);
    RECT windowRectangle;
    GetClientRect(windowHandle, &windowRectangle);
    HBITMAP bitmapHandle = CreateCompatibleBitmap(deviceContextHandle, windowRectangle.right, windowRectangle.bottom);
    BITMAP deviceContextBitmap;
    SelectObject(deviceContext, bitmapHandle);
    BitBlt(deviceContext, 0, 0, windowRectangle.right, windowRectangle.bottom, deviceContextHandle, 0, 0, SRCCOPY);
    int objectGotSuccessfully = GetObject(bitmapHandle, sizeof(BITMAP), &deviceContextBitmap);
    if (objectGotSuccessfully)
    {
        bitmapFile.bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapFile.bitmapInfo.bmiHeader.biWidth = deviceContextBitmap.bmWidth;
        bitmapFile.bitmapInfo.bmiHeader.biHeight = deviceContextBitmap.bmHeight;
        bitmapFile.bitmapInfo.bmiHeader.biPlanes = deviceContextBitmap.bmPlanes;
        bitmapFile.bitmapInfo.bmiHeader.biBitCount = deviceContextBitmap.bmBitsPixel;
        bitmapFile.bitmapInfo.bmiHeader.biCompression = BI_RGB;
        int imageSize = deviceContextBitmap.bmWidth * deviceContextBitmap.bmHeight * deviceContextBitmap.bmBitsPixel / 8;
        bitmapFile.bitmapInfo.bmiHeader.biSizeImage = deviceContextBitmap.bmHeight * deviceContextBitmap.bmWidth;
        bitmapFile.bitmapData.resize(imageSize);
        int offsetSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        int fileSize = offsetSize + imageSize;
        bitmapFile.bitmapFileHeader.bfSize = fileSize;
        bitmapFile.bitmapFileHeader.bfType = 0x4D42;
        bitmapFile.bitmapFileHeader.bfOffBits = offsetSize;
        bitmapFile.bitmapFileHeader.bfReserved1 = 0;
        bitmapFile.bitmapFileHeader.bfReserved2 = 0;
        int DIBitsGotSuccessfully = GetDIBits(deviceContextHandle, bitmapHandle, 0, deviceContextBitmap.bmHeight, &bitmapFile.bitmapData[0], &bitmapFile.bitmapInfo, DIB_RGB_COLORS);
        if (DIBitsGotSuccessfully)
            bitmapFile.SetValid();
    }

    return bitmapFile;
}

Matrix::Matrix<Pixel> CreateMatrixFromBitmap(Bitmap::File bitmapFile)
{
    Matrix::Matrix<Pixel> imageMatrix(bitmapFile.bitmapInfo.bmiHeader.biHeight, bitmapFile.bitmapInfo.bmiHeader.biWidth);
    if (bitmapFile.bitmapInfo.bmiHeader.biBitCount == 32)
    {
        int k = 0;
        for (int i = 0; i < imageMatrix.rows(); i++)
            for (int j = 0; j < imageMatrix.cols(); j++)
            {
                imageMatrix[i][j].blue = bitmapFile.bitmapData[k];
                imageMatrix[i][j].green = bitmapFile.bitmapData[k + 1];
                imageMatrix[i][j].red = bitmapFile.bitmapData[k + 2];
                imageMatrix[i][j].alpha = bitmapFile.bitmapData[k + 3];
                k += 4;
            }
    }

    else if (bitmapFile.bitmapInfo.bmiHeader.biBitCount == 24)
    {
        int k = 0;
        for (int i = 0; i < imageMatrix.rows(); i++)
            for (int j = 0; j < imageMatrix.cols(); j++)
            {
                imageMatrix[i][j].blue = bitmapFile.bitmapData[k];
                imageMatrix[i][j].green = bitmapFile.bitmapData[k + 1];
                imageMatrix[i][j].red = bitmapFile.bitmapData[k + 2];
                imageMatrix[i][j].alpha = 0;
                k += 3;
            }
    }

    return imageMatrix;
}

Bitmap::File CreateBitmapFromMatrix(Matrix::Matrix<Pixel> imageMatrix)
{
    Bitmap::File bitmapFile;
    bitmapFile.bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapFile.bitmapInfo.bmiHeader.biWidth = imageMatrix.cols();
    bitmapFile.bitmapInfo.bmiHeader.biHeight = imageMatrix.rows();
    bitmapFile.bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapFile.bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapFile.bitmapInfo.bmiHeader.biCompression = BI_RGB;
    int imageSize = imageMatrix.size() * 32 / 8;
    bitmapFile.bitmapInfo.bmiHeader.biSizeImage = imageMatrix.size();
    bitmapFile.bitmapData.resize(imageSize);
    int offsetSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    int fileSize = offsetSize + imageSize;
    bitmapFile.bitmapFileHeader.bfSize = fileSize;
    bitmapFile.bitmapFileHeader.bfType = 0x4D42;
    bitmapFile.bitmapFileHeader.bfOffBits = offsetSize;
    bitmapFile.bitmapFileHeader.bfReserved1 = 0;
    bitmapFile.bitmapFileHeader.bfReserved2 = 0;
    int k = 0;
    for (int i = 0; i < imageMatrix.rows(); i++)
        for (int j = 0; j < imageMatrix.cols(); j++)
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

Bitmap::File GreyscaleImage(Bitmap::File bitmapFile)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = GreyScalePixel(pixels);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ShrinkImage(Bitmap::File bitmapFile, int scaleFactor)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    Matrix::Matrix<Pixel> shrunkenMatrix(imageMatrix.rows() / scaleFactor, imageMatrix.cols() / scaleFactor);
    for (int i = 0; i < shrunkenMatrix.rows(); i++)
        for (int j = 0; j < shrunkenMatrix.cols(); j++)
        {
            int averageRed = 0;
            int averageGreen = 0;
            int averageBlue = 0;
            int averageAlpha = 0;
            for (int k = 0; (k < scaleFactor) && ((k + (i * scaleFactor)) < imageMatrix.rows()); k++)
                for (int l = 0; (l < scaleFactor) && ((l + (j * scaleFactor)) < imageMatrix.cols()); l++)
                {
                    averageRed += imageMatrix[k + (i * scaleFactor)][l + (j * scaleFactor)].red;
                    averageGreen += imageMatrix[k + (i * scaleFactor)][l + (j * scaleFactor)].green;
                    averageBlue += imageMatrix[k + (i * scaleFactor)][l + (j * scaleFactor)].blue;
                    averageAlpha += imageMatrix[k + (i * scaleFactor)][l + (j * scaleFactor)].alpha;
                }
            averageRed = averageRed / (scaleFactor * scaleFactor);
            averageGreen = averageGreen / (scaleFactor * scaleFactor);
            averageBlue = averageBlue / (scaleFactor * scaleFactor);
            averageAlpha = averageAlpha / (scaleFactor * scaleFactor);
            shrunkenMatrix[i][j].red = averageRed;
            shrunkenMatrix[i][j].green = averageGreen;
            shrunkenMatrix[i][j].blue = averageBlue;
            shrunkenMatrix[i][j].alpha = averageAlpha;
        }
    bitmapFile = CreateBitmapFromMatrix(shrunkenMatrix);
    return bitmapFile;
}

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

Bitmap::File ChangeImageBrightness(Bitmap::File bitmapFile, float brightness)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelBrightness(pixels, brightness);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageSaturation(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelSaturation(pixels, saturation);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

Pixel GreyScalePixel(Pixel pixel)
{
    int average = (pixel.red + pixel.blue + pixel.green) / 3;
    pixel.blue = average;
    pixel.red = average;
    pixel.green = average;
    return pixel;
}

Pixel ChangePixelBrightness(Pixel pixel, float brightness)
{
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    int newAverage = (int)(average * brightness);
    if (((pixel.red - average) + newAverage <= 255) && ((pixel.red - average) + newAverage >= 0))
        pixel.red = (pixel.red - average) + newAverage;
    else if (((pixel.red - average) + newAverage < 0))
        pixel.red = 0;
    else
        pixel.red = 255;

    if (((pixel.green - average) + newAverage <= 255) && ((pixel.green - average) + newAverage >= 0))
        pixel.green = (pixel.green - average) + newAverage;
    else if (((pixel.green - average) + newAverage < 0))
        pixel.green = 0;
    else
        pixel.green = 255;

    if (((pixel.blue - average) + newAverage <= 255) && ((pixel.blue - average) + newAverage >= 0))
        pixel.blue = (pixel.blue - average) + newAverage;
    else if (((pixel.blue - average) + newAverage < 0))
        pixel.blue = 0;
    else
        pixel.blue = 255;

    return pixel;
}

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

Bitmap::File ChangeImageContrast(Bitmap::File bitmapFile, float contrast)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelContrast(pixels, contrast);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Pixel ChangePixelContrast(Pixel pixel, float contrast)
{
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    int newAverage = (int)((erf(((average / 128) - 1) * contrast) + 1) * average);
    if (((pixel.red - average) + newAverage <= 255) && ((pixel.red - average) + newAverage >= 0))
        pixel.red = (pixel.red - average) + newAverage;
    else if (((pixel.red - average) + newAverage < 0))
        pixel.red = 0;
    else
        pixel.red = 255;

    if (((pixel.green - average) + newAverage <= 255) && ((pixel.green - average) + newAverage >= 0))
        pixel.green = (pixel.green - average) + newAverage;
    else if (((pixel.green - average) + newAverage < 0))
        pixel.green = 0;
    else
        pixel.green = 255;

    if (((pixel.blue - average) + newAverage <= 255) && ((pixel.blue - average) + newAverage >= 0))
        pixel.blue = (pixel.blue - average) + newAverage;
    else if (((pixel.blue - average) + newAverage < 0))
        pixel.blue = 0;
    else
        pixel.blue = 255;

    return pixel;
}

Pixel ChangePixelSaturationBlue(Pixel pixel, float saturation)
{
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    if ((pixel.blue >= pixel.red) && (pixel.blue >= pixel.red))
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

Pixel ChangePixelSaturationGreen(Pixel pixel, float saturation)
{
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    if ((pixel.green >= pixel.red) && (pixel.green >= pixel.blue))
    {
        if ((((pixel.green - average) * saturation + average) <= 255) && (((pixel.green - average) * saturation + average) >= 0))
            pixel.green = (BYTE)((pixel.green - average) * saturation + average);
        else if ((((pixel.green - average) * saturation + average) > 255))
            pixel.green = 255;
        else
            pixel.green = 0;
    }
    return pixel;
}

Pixel ChangePixelSaturationRed(Pixel pixel, float saturation)
{
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    if ((pixel.red >= pixel.blue) && (pixel.red >= pixel.green))
    {
        if ((((pixel.red - average) * saturation + average) <= 255) && (((pixel.red - average) * saturation + average) >= 0))
            pixel.red = (BYTE)((pixel.red - average) * saturation + average);
        else if ((((pixel.red - average) * saturation + average) > 255))
            pixel.red = 255;
        else
            pixel.red = 0;
    }
    return pixel;
}

Pixel ChangePixelSaturationMagenta(Pixel pixel, float saturation)
{
    int average = (pixel.red + pixel.blue + pixel.green) / 3;
    int magenta = (std::min)(pixel.red, pixel.blue);
    int redOffset = pixel.red - magenta;
    int blueOffset = pixel.blue - magenta;
    if (magenta >= average)
        magenta = (int)((magenta - average) * saturation + average);
    if ((redOffset + magenta <= 255) && (redOffset + magenta >= 0))
        pixel.red = redOffset + magenta;
    else if (redOffset + magenta > 255)
        pixel.red = 255;
    else
        pixel.red = 0;

    if ((blueOffset + magenta <= 255) && (blueOffset + magenta >= 0))
        pixel.blue = blueOffset + magenta;
    else if (blueOffset + magenta > 255)
        pixel.blue = 255;
    else
        pixel.blue = 0;

    return pixel;
}

Pixel ChangePixelSaturationYellow(Pixel pixel, float saturation)
{
    int average = (pixel.red + pixel.blue + pixel.green) / 3;
    int yellow = (std::min)(pixel.red, pixel.green);
    int redOffset = pixel.red - yellow;
    int greenOffset = pixel.green - yellow;
    if (yellow >= average)
        yellow = (int)((yellow - average) * saturation + average);
    if ((redOffset + yellow <= 255) && (redOffset + yellow >= 0))
        pixel.red = redOffset + yellow;
    else if (redOffset + yellow > 255)
        pixel.red = 255;
    else
        pixel.red = 0;

    if ((greenOffset + yellow <= 255) && (greenOffset + yellow >= 0))
        pixel.green = greenOffset + yellow;
    else if (greenOffset + yellow > 255)
        pixel.green = 255;
    else
        pixel.green = 0;

    return pixel;
}

Pixel ChangePixelSaturationCyan(Pixel pixel, float saturation)
{
    int average = (pixel.red + pixel.blue + pixel.green) / 3;
    int cyan = (std::min)(pixel.blue, pixel.green);
    int greenOffset = pixel.green - cyan;
    int blueOffset = pixel.blue - cyan;
    if (cyan >= average)
        cyan = (int)((cyan - average) * saturation + average);
    if ((greenOffset + cyan <= 255) && (greenOffset + cyan >= 0))
        pixel.green = greenOffset + cyan;
    else if (greenOffset + cyan > 255)
        pixel.green = 255;
    else
        pixel.green = 0;

    if ((blueOffset + cyan <= 255) && (blueOffset + cyan >= 0))
        pixel.blue = blueOffset + cyan;
    else if (blueOffset + cyan > 255)
        pixel.blue = 255;
    else
        pixel.blue = 0;

    return pixel;
}

Bitmap::File ChangeImageSaturationBlue(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelSaturationBlue(pixels, saturation);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

Bitmap::File ChangeImageSaturationGreen(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelSaturationGreen(pixels, saturation);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

Bitmap::File ChangeImageSaturationRed(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelSaturationRed(pixels, saturation);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

Bitmap::File ChangeImageSaturationMagenta(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelSaturationMagenta(pixels, saturation);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

Bitmap::File ChangeImageSaturationYellow(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelSaturationYellow(pixels, saturation);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

Bitmap::File ChangeImageSaturationCyan(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelSaturationCyan(pixels, saturation);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

Pixel ChangePixelLuminanceBlue(Pixel pixel, float luminance)
{
    if ((pixel.blue >= pixel.red) && (pixel.blue >= pixel.green))
    {
        int average = (pixel.red + pixel.green + pixel.blue) / 3;
        int newAverage = (int)(average * luminance);
        if (((pixel.red - average) + newAverage <= 255) && ((pixel.red - average) + newAverage >= 0))
            pixel.red = (pixel.red - average) + newAverage;
        else if (((pixel.red - average) + newAverage < 0))
            pixel.red = 0;
        else
            pixel.red = 255;

        if (((pixel.green - average) + newAverage <= 255) && ((pixel.green - average) + newAverage >= 0))
            pixel.green = (pixel.green - average) + newAverage;
        else if (((pixel.green - average) + newAverage < 0))
            pixel.green = 0;
        else
            pixel.green = 255;

        if (((pixel.blue - average) + newAverage <= 255) && ((pixel.blue - average) + newAverage >= 0))
            pixel.blue = (pixel.blue - average) + newAverage;
        else if (((pixel.blue - average) + newAverage < 0))
            pixel.blue = 0;
        else
            pixel.blue = 255;
    }

    return pixel;
}

Pixel ChangePixelLuminanceGreen(Pixel pixel, float luminance)
{
    if ((pixel.green >= pixel.red) && (pixel.green >= pixel.blue))
    {
        int average = (pixel.red + pixel.green + pixel.blue) / 3;
        int newAverage = (int)(average * luminance);
        if (((pixel.red - average) + newAverage <= 255) && ((pixel.red - average) + newAverage >= 0))
            pixel.red = (pixel.red - average) + newAverage;
        else if (((pixel.red - average) + newAverage < 0))
            pixel.red = 0;
        else
            pixel.red = 255;

        if (((pixel.green - average) + newAverage <= 255) && ((pixel.green - average) + newAverage >= 0))
            pixel.green = (pixel.green - average) + newAverage;
        else if (((pixel.green - average) + newAverage < 0))
            pixel.green = 0;
        else
            pixel.green = 255;

        if (((pixel.blue - average) + newAverage <= 255) && ((pixel.blue - average) + newAverage >= 0))
            pixel.blue = (pixel.blue - average) + newAverage;
        else if (((pixel.blue - average) + newAverage < 0))
            pixel.blue = 0;
        else
            pixel.blue = 255;
    }

    return pixel;
}

Pixel ChangePixelLuminanceRed(Pixel pixel, float luminance)
{
    if ((pixel.red >= pixel.green) && (pixel.red >= pixel.blue))
    {
        int average = (pixel.red + pixel.green + pixel.blue) / 3;
        int newAverage = (int)(average * luminance);
        if (((pixel.red - average) + newAverage <= 255) && ((pixel.red - average) + newAverage >= 0))
            pixel.red = (pixel.red - average) + newAverage;
        else if (((pixel.red - average) + newAverage < 0))
            pixel.red = 0;
        else
            pixel.red = 255;

        if (((pixel.green - average) + newAverage <= 255) && ((pixel.green - average) + newAverage >= 0))
            pixel.green = (pixel.green - average) + newAverage;
        else if (((pixel.green - average) + newAverage < 0))
            pixel.green = 0;
        else
            pixel.green = 255;

        if (((pixel.blue - average) + newAverage <= 255) && ((pixel.blue - average) + newAverage >= 0))
            pixel.blue = (pixel.blue - average) + newAverage;
        else if (((pixel.blue - average) + newAverage < 0))
            pixel.blue = 0;
        else
            pixel.blue = 255;
    }

    return pixel;
}

Pixel ChangePixelLuminanceMagenta(Pixel pixel, float luminance)
{

    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    int magenta = (std::min)(pixel.red, pixel.blue);
    if (magenta > average)
    {
        int newAverage = (int)(average * luminance);
        if (((pixel.red - average) + newAverage <= 255) && ((pixel.red - average) + newAverage >= 0))
            pixel.red = (pixel.red - average) + newAverage;
        else if (((pixel.red - average) + newAverage < 0))
            pixel.red = 0;
        else
            pixel.red = 255;

        if (((pixel.green - average) + newAverage <= 255) && ((pixel.green - average) + newAverage >= 0))
            pixel.green = (pixel.green - average) + newAverage;
        else if (((pixel.green - average) + newAverage < 0))
            pixel.green = 0;
        else
            pixel.green = 255;

        if (((pixel.blue - average) + newAverage <= 255) && ((pixel.blue - average) + newAverage >= 0))
            pixel.blue = (pixel.blue - average) + newAverage;
        else if (((pixel.blue - average) + newAverage < 0))
            pixel.blue = 0;
        else
            pixel.blue = 255;
    }

    return pixel;
}

Pixel ChangePixelLuminanceYellow(Pixel pixel, float luminance)
{
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    int yellow = (std::min)(pixel.red, pixel.green);
    if (yellow > average)
    {
        int newAverage = (int)(average * luminance);
        if (((pixel.red - average) + newAverage <= 255) && ((pixel.red - average) + newAverage >= 0))
            pixel.red = (pixel.red - average) + newAverage;
        else if (((pixel.red - average) + newAverage < 0))
            pixel.red = 0;
        else
            pixel.red = 255;

        if (((pixel.green - average) + newAverage <= 255) && ((pixel.green - average) + newAverage >= 0))
            pixel.green = (pixel.green - average) + newAverage;
        else if (((pixel.green - average) + newAverage < 0))
            pixel.green = 0;
        else
            pixel.green = 255;

        if (((pixel.blue - average) + newAverage <= 255) && ((pixel.blue - average) + newAverage >= 0))
            pixel.blue = (pixel.blue - average) + newAverage;
        else if (((pixel.blue - average) + newAverage < 0))
            pixel.blue = 0;
        else
            pixel.blue = 255;
    }

    return pixel;
}

Pixel ChangePixelLuminanceCyan(Pixel pixel, float luminance)
{
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    int cyan = (std::min)(pixel.green, pixel.blue);
    if (cyan > average)
    {
        int newAverage = (int)(average * luminance);
        if (((pixel.red - average) + newAverage <= 255) && ((pixel.red - average) + newAverage >= 0))
            pixel.red = (pixel.red - average) + newAverage;
        else if (((pixel.red - average) + newAverage < 0))
            pixel.red = 0;
        else
            pixel.red = 255;

        if (((pixel.green - average) + newAverage <= 255) && ((pixel.green - average) + newAverage >= 0))
            pixel.green = (pixel.green - average) + newAverage;
        else if (((pixel.green - average) + newAverage < 0))
            pixel.green = 0;
        else
            pixel.green = 255;

        if (((pixel.blue - average) + newAverage <= 255) && ((pixel.blue - average) + newAverage >= 0))
            pixel.blue = (pixel.blue - average) + newAverage;
        else if (((pixel.blue - average) + newAverage < 0))
            pixel.blue = 0;
        else
            pixel.blue = 255;
    }

    return pixel;
}

Bitmap::File ChangeImageLuminanceBlue(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelLuminanceBlue(pixels, luminance);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

Bitmap::File ChangeImageLuminanceGreen(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelLuminanceGreen(pixels, luminance);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

Bitmap::File ChangeImageLuminanceRed(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelLuminanceRed(pixels, luminance);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

Bitmap::File ChangeImageLuminanceMagenta(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelLuminanceMagenta(pixels, luminance);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

Bitmap::File ChangeImageLuminanceYellow(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelLuminanceYellow(pixels, luminance);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}

Bitmap::File ChangeImageLuminanceCyan(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto rows : imageMatrix)
        for (auto &pixels : rows)
            pixels = ChangePixelLuminanceCyan(pixels, luminance);
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);

    return bitmapFile;
}