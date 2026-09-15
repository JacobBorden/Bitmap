#include "bitmap.h"
#include <iostream>

#if defined(_WIN32)
Bitmap::File ScreenShotWindow(HWND windowHandle)
{
    Bitmap::File bitmapFile;
    if (!windowHandle)
    {
        return bitmapFile;
    }

    OpenIcon(windowHandle);
    BringWindowToTop(windowHandle);
    SetActiveWindow(windowHandle);
    HDC deviceContextHandle = GetDC(windowHandle);
    if (!deviceContextHandle)
    {
        return bitmapFile;
    }

    HDC deviceContext = CreateCompatibleDC(deviceContextHandle);
    RECT windowRectangle;
    GetClientRect(windowHandle, &windowRectangle);
    int width = windowRectangle.right - windowRectangle.left;
    int height = windowRectangle.bottom - windowRectangle.top;
    if (width <= 0 || height <= 0)
    {
        DeleteDC(deviceContext);
        ReleaseDC(windowHandle, deviceContextHandle);
        return bitmapFile;
    }

    HBITMAP bitmapHandle = CreateCompatibleBitmap(deviceContextHandle, width, height);
    BITMAP deviceContextBitmap;
    SelectObject(deviceContext, bitmapHandle);
    BitBlt(deviceContext, 0, 0, width, height, deviceContextHandle, 0, 0, SRCCOPY);
    int objectGotSuccessfully = GetObject(bitmapHandle, sizeof(BITMAP), &deviceContextBitmap);
    if (objectGotSuccessfully)
    {
        bitmapFile.bitmapInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapFile.bitmapInfoHeader.biWidth = deviceContextBitmap.bmWidth;
        bitmapFile.bitmapInfoHeader.biHeight = deviceContextBitmap.bmHeight;
        bitmapFile.bitmapInfoHeader.biPlanes = deviceContextBitmap.bmPlanes;
        bitmapFile.bitmapInfoHeader.biBitCount = deviceContextBitmap.bmBitsPixel;
        bitmapFile.bitmapInfoHeader.biCompression = 0; // BI_RGB
        int imageSize = deviceContextBitmap.bmWidth * deviceContextBitmap.bmHeight * deviceContextBitmap.bmBitsPixel / 8;
        bitmapFile.bitmapInfoHeader.biSizeImage = imageSize;
        bitmapFile.bitmapData.resize(imageSize);
        int offsetSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        int fileSize = offsetSize + imageSize;
        bitmapFile.bitmapFileHeader.bfSize = fileSize;
        bitmapFile.bitmapFileHeader.bfType = 0x4D42;
        bitmapFile.bitmapFileHeader.bfOffBits = offsetSize;
        bitmapFile.bitmapFileHeader.bfReserved1 = 0;
        bitmapFile.bitmapFileHeader.bfReserved2 = 0;

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = deviceContextBitmap.bmWidth;
        bmi.bmiHeader.biHeight = deviceContextBitmap.bmHeight;
        bmi.bmiHeader.biPlanes = deviceContextBitmap.bmPlanes;
        bmi.bmiHeader.biBitCount = deviceContextBitmap.bmBitsPixel;
        bmi.bmiHeader.biCompression = BI_RGB;
        int DIBitsGotSuccessfully = GetDIBits(deviceContextHandle, bitmapHandle, 0, deviceContextBitmap.bmHeight, &bitmapFile.bitmapData[0], &bmi, DIB_RGB_COLORS);
        if (DIBitsGotSuccessfully)
        {
            bitmapFile.SetValid();
        }
    }

    DeleteObject(bitmapHandle);
    DeleteDC(deviceContext);
    ReleaseDC(windowHandle, deviceContextHandle);

    return bitmapFile;
}
#endif

Matrix::Matrix<Pixel> CreateMatrixFromBitmap(Bitmap::File bitmapFile)
{
    Matrix::Matrix<Pixel> imageMatrix(static_cast<int>(bitmapFile.bitmapInfoHeader.biHeight),
                                      static_cast<int>(bitmapFile.bitmapInfoHeader.biWidth));
    if (bitmapFile.bitmapInfoHeader.biBitCount == 32)
    {
        int k = 0;
        for (size_t i = 0; i < imageMatrix.rows(); i++)
        {
            for (size_t j = 0; j < imageMatrix.cols(); j++)
            {
                if (k + 3 < static_cast<int>(bitmapFile.bitmapData.size()))
                {
                    imageMatrix[i][j].blue = bitmapFile.bitmapData[k];
                    imageMatrix[i][j].green = bitmapFile.bitmapData[k + 1];
                    imageMatrix[i][j].red = bitmapFile.bitmapData[k + 2];
                    imageMatrix[i][j].alpha = bitmapFile.bitmapData[k + 3];
                }
                k += 4;
            }
        }
    }
    else if (bitmapFile.bitmapInfoHeader.biBitCount == 24)
    {
        int k = 0;
        for (size_t i = 0; i < imageMatrix.rows(); i++)
        {
            for (size_t j = 0; j < imageMatrix.cols(); j++)
            {
                if (k + 2 < static_cast<int>(bitmapFile.bitmapData.size()))
                {
                    imageMatrix[i][j].blue = bitmapFile.bitmapData[k];
                    imageMatrix[i][j].green = bitmapFile.bitmapData[k + 1];
                    imageMatrix[i][j].red = bitmapFile.bitmapData[k + 2];
                    imageMatrix[i][j].alpha = 0;
                }
                k += 3;
            }
        }
    }

    return imageMatrix;
}

Bitmap::File CreateBitmapFromMatrix(const Matrix::Matrix<Pixel>& imageMatrix)
{
    Bitmap::File bitmapFile;
    bitmapFile.bitmapInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapFile.bitmapInfoHeader.biWidth = static_cast<LONG>(imageMatrix.cols());
    bitmapFile.bitmapInfoHeader.biHeight = static_cast<LONG>(imageMatrix.rows());
    bitmapFile.bitmapInfoHeader.biPlanes = 1;
    bitmapFile.bitmapInfoHeader.biBitCount = 32;
    bitmapFile.bitmapInfoHeader.biCompression = 0; // BI_RGB
    int imageSize = static_cast<int>(imageMatrix.size() * 32 / 8);
    bitmapFile.bitmapInfoHeader.biSizeImage = static_cast<DWORD>(imageSize);
    bitmapFile.bitmapData.resize(imageSize);
    int offsetSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    int fileSize = offsetSize + imageSize;
    bitmapFile.bitmapFileHeader.bfSize = fileSize;
    bitmapFile.bitmapFileHeader.bfType = 0x4D42;
    bitmapFile.bitmapFileHeader.bfOffBits = offsetSize;
    bitmapFile.bitmapFileHeader.bfReserved1 = 0;
    bitmapFile.bitmapFileHeader.bfReserved2 = 0;
    int k = 0;
    for (size_t i = 0; i < imageMatrix.rows(); i++)
    {
        for (size_t j = 0; j < imageMatrix.cols(); j++)
        {
            bitmapFile.bitmapData[k] = imageMatrix[i][j].blue;
            bitmapFile.bitmapData[k + 1] = imageMatrix[i][j].green;
            bitmapFile.bitmapData[k + 2] = imageMatrix[i][j].red;
            bitmapFile.bitmapData[k + 3] = imageMatrix[i][j].alpha;
            k += 4;
        }
    }
    bitmapFile.SetValid();
    return bitmapFile;
}

Bitmap::File GreyscaleImage(Bitmap::File bitmapFile)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = GreyScalePixel(pixel);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ShrinkImage(Bitmap::File bitmapFile, int scaleFactor)
{
    if (scaleFactor <= 0)
    {
        return bitmapFile;
    }

    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    int newRows = static_cast<int>(imageMatrix.rows()) / scaleFactor;
    int newCols = static_cast<int>(imageMatrix.cols()) / scaleFactor;
    Matrix::Matrix<Pixel> shrunkenMatrix(newRows, newCols);
    for (size_t i = 0; i < shrunkenMatrix.rows(); i++)
    {
        for (size_t j = 0; j < shrunkenMatrix.cols(); j++)
        {
            int averageRed = 0;
            int averageGreen = 0;
            int averageBlue = 0;
            int averageAlpha = 0;
            for (size_t k = 0; (static_cast<int>(k) < scaleFactor) && ((k + (i * scaleFactor)) < imageMatrix.rows()); k++)
            {
                for (size_t l = 0; (static_cast<int>(l) < scaleFactor) && ((l + (j * scaleFactor)) < imageMatrix.cols()); l++)
                {
                    averageRed += imageMatrix[k + (i * scaleFactor)][l + (j * scaleFactor)].red;
                    averageGreen += imageMatrix[k + (i * scaleFactor)][l + (j * scaleFactor)].green;
                    averageBlue += imageMatrix[k + (i * scaleFactor)][l + (j * scaleFactor)].blue;
                    averageAlpha += imageMatrix[k + (i * scaleFactor)][l + (j * scaleFactor)].alpha;
                }
            }
            averageRed = averageRed / (scaleFactor * scaleFactor);
            averageGreen = averageGreen / (scaleFactor * scaleFactor);
            averageBlue = averageBlue / (scaleFactor * scaleFactor);
            averageAlpha = averageAlpha / (scaleFactor * scaleFactor);
            shrunkenMatrix[i][j].red = static_cast<BYTE>(averageRed);
            shrunkenMatrix[i][j].green = static_cast<BYTE>(averageGreen);
            shrunkenMatrix[i][j].blue = static_cast<BYTE>(averageBlue);
            shrunkenMatrix[i][j].alpha = static_cast<BYTE>(averageAlpha);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(shrunkenMatrix);
    return bitmapFile;
}

Bitmap::File RotateImageCounterClockwise(Bitmap::File bitmapFile)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    Matrix::Matrix<Pixel> rotatedMatrix(static_cast<int>(imageMatrix.cols()), static_cast<int>(imageMatrix.rows()));
    for (size_t i = 0; i < imageMatrix.rows(); i++)
    {
        for (size_t j = 0; j < imageMatrix.cols(); j++)
        {
            rotatedMatrix[j][rotatedMatrix.cols() - i - 1] = imageMatrix[i][j];
        }
    }
    bitmapFile = CreateBitmapFromMatrix(rotatedMatrix);
    return bitmapFile;
}

Bitmap::File RotateImageClockwise(Bitmap::File bitmapFile)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    Matrix::Matrix<Pixel> rotatedMatrix(static_cast<int>(imageMatrix.cols()), static_cast<int>(imageMatrix.rows()));
    for (size_t i = 0; i < imageMatrix.rows(); i++)
    {
        for (size_t j = 0; j < imageMatrix.cols(); j++)
        {
            rotatedMatrix[rotatedMatrix.rows() - j - 1][i] = imageMatrix[i][j];
        }
    }
    bitmapFile = CreateBitmapFromMatrix(rotatedMatrix);
    return bitmapFile;
}

Bitmap::File MirrorImage(Bitmap::File bitmapFile)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    Matrix::Matrix<Pixel> mirroredMatrix(static_cast<int>(imageMatrix.rows()), static_cast<int>(imageMatrix.cols()));
    for (size_t i = 0; i < imageMatrix.rows(); i++)
    {
        for (size_t j = 0; j < imageMatrix.cols(); j++)
        {
            mirroredMatrix[i][mirroredMatrix.cols() - j - 1] = imageMatrix[i][j];
        }
    }
    bitmapFile = CreateBitmapFromMatrix(mirroredMatrix);
    return bitmapFile;
}

Bitmap::File FlipImage(Bitmap::File bitmapFile)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    Matrix::Matrix<Pixel> flippedMatrix(static_cast<int>(imageMatrix.rows()), static_cast<int>(imageMatrix.cols()));
    for (size_t i = 0; i < imageMatrix.rows(); i++)
    {
        for (size_t j = 0; j < imageMatrix.cols(); j++)
        {
            flippedMatrix[flippedMatrix.rows() - i - 1][j] = imageMatrix[i][j];
        }
    }
    bitmapFile = CreateBitmapFromMatrix(flippedMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageBrightness(Bitmap::File bitmapFile, float brightness)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelBrightness(pixel, brightness);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageContrast(Bitmap::File bitmapFile, float contrast)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelContrast(pixel, contrast);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageContrastRed(Bitmap::File bitmapFile, float contrast)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelContrastRed(pixel, contrast);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageContrastGreen(Bitmap::File bitmapFile, float contrast)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelContrastGreen(pixel, contrast);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageContrastBlue(Bitmap::File bitmapFile, float contrast)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelContrastBlue(pixel, contrast);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageContrastMagenta(Bitmap::File bitmapFile, float contrast)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelContrastMagenta(pixel, contrast);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageContrastYellow(Bitmap::File bitmapFile, float contrast)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelContrastYellow(pixel, contrast);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageContrastCyan(Bitmap::File bitmapFile, float contrast)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelContrastCyan(pixel, contrast);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageSaturation(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelSaturation(pixel, saturation);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageSaturationBlue(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelSaturationBlue(pixel, saturation);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageSaturationGreen(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelSaturationGreen(pixel, saturation);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageSaturationRed(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelSaturationRed(pixel, saturation);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageSaturationMagenta(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelSaturationMagenta(pixel, saturation);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageSaturationYellow(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelSaturationYellow(pixel, saturation);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageSaturationCyan(Bitmap::File bitmapFile, float saturation)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelSaturationCyan(pixel, saturation);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageLuminanceBlue(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelLuminanceBlue(pixel, luminance);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageLuminanceGreen(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelLuminanceGreen(pixel, luminance);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageLuminanceRed(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelLuminanceRed(pixel, luminance);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageLuminanceMagenta(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelLuminanceMagenta(pixel, luminance);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageLuminanceYellow(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelLuminanceYellow(pixel, luminance);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Bitmap::File ChangeImageLuminanceCyan(Bitmap::File bitmapFile, float luminance)
{
    Matrix::Matrix<Pixel> imageMatrix = CreateMatrixFromBitmap(bitmapFile);
    for (auto &row : imageMatrix)
    {
        for (auto &pixel : row)
        {
            pixel = ChangePixelLuminanceCyan(pixel, luminance);
        }
    }
    bitmapFile = CreateBitmapFromMatrix(imageMatrix);
    return bitmapFile;
}

Pixel GreyScalePixel(Pixel pixel)
{
    int average = (pixel.red + pixel.blue + pixel.green) / 3;
    pixel.blue = static_cast<BYTE>(average);
    pixel.red = static_cast<BYTE>(average);
    pixel.green = static_cast<BYTE>(average);
    return pixel;
}

Pixel ChangePixelBrightness(Pixel pixel, float brightness)
{
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    int newAverage = static_cast<int>(average * brightness);

    int r = (pixel.red - average) + newAverage;
    if (r < 0) r = 0; else if (r > 255) r = 255;
    pixel.red = static_cast<BYTE>(r);

    int g = (pixel.green - average) + newAverage;
    if (g < 0) g = 0; else if (g > 255) g = 255;
    pixel.green = static_cast<BYTE>(g);

    int b = (pixel.blue - average) + newAverage;
    if (b < 0) b = 0; else if (b > 255) b = 255;
    pixel.blue = static_cast<BYTE>(b);

    return pixel;
}

Pixel ChangePixelContrast(Pixel pixel, float contrast)
{
    int new_red = static_cast<int>(128 + (pixel.red - 128) * contrast);
    if (new_red < 0) new_red = 0; else if (new_red > 255) new_red = 255;
    pixel.red = static_cast<BYTE>(new_red);

    int new_green = static_cast<int>(128 + (pixel.green - 128) * contrast);
    if (new_green < 0) new_green = 0; else if (new_green > 255) new_green = 255;
    pixel.green = static_cast<BYTE>(new_green);

    int new_blue = static_cast<int>(128 + (pixel.blue - 128) * contrast);
    if (new_blue < 0) new_blue = 0; else if (new_blue > 255) new_blue = 255;
    pixel.blue = static_cast<BYTE>(new_blue);

    return pixel;
}

Pixel ChangePixelContrastRed(Pixel pixel, float contrast)
{
    int new_red = static_cast<int>(128 + (pixel.red - 128) * contrast);
    if (new_red < 0) new_red = 0; else if (new_red > 255) new_red = 255;
    pixel.red = static_cast<BYTE>(new_red);
    return pixel;
}

Pixel ChangePixelContrastGreen(Pixel pixel, float contrast)
{
    int new_green = static_cast<int>(128 + (pixel.green - 128) * contrast);
    if (new_green < 0) new_green = 0; else if (new_green > 255) new_green = 255;
    pixel.green = static_cast<BYTE>(new_green);
    return pixel;
}

Pixel ChangePixelContrastBlue(Pixel pixel, float contrast)
{
    int new_blue = static_cast<int>(128 + (pixel.blue - 128) * contrast);
    if (new_blue < 0) new_blue = 0; else if (new_blue > 255) new_blue = 255;
    pixel.blue = static_cast<BYTE>(new_blue);
    return pixel;
}

Pixel ChangePixelContrastMagenta(Pixel pixel, float contrast)
{
    int magenta = (std::min)(static_cast<int>(pixel.red), static_cast<int>(pixel.blue));
    int redoffset = pixel.red - magenta;
    int blueoffset = pixel.blue - magenta;

    int new_red = static_cast<int>(128 + (magenta - 128) * contrast) + redoffset;
    if (new_red < 0) new_red = 0; else if (new_red > 255) new_red = 255;
    pixel.red = static_cast<BYTE>(new_red);

    int new_blue = static_cast<int>(128 + (magenta - 128) * contrast) + blueoffset;
    if (new_blue < 0) new_blue = 0; else if (new_blue > 255) new_blue = 255;
    pixel.blue = static_cast<BYTE>(new_blue);

    return pixel;
}

Pixel ChangePixelContrastYellow(Pixel pixel, float contrast)
{
    int yellow = (std::min)(static_cast<int>(pixel.red), static_cast<int>(pixel.green));
    int redoffset = pixel.red - yellow;
    int greenoffset = pixel.green - yellow;

    int new_red = static_cast<int>(128 + (yellow - 128) * contrast) + redoffset;
    if (new_red < 0) new_red = 0; else if (new_red > 255) new_red = 255;
    pixel.red = static_cast<BYTE>(new_red);

    int new_green = static_cast<int>(128 + (yellow - 128) * contrast) + greenoffset;
    if (new_green < 0) new_green = 0; else if (new_green > 255) new_green = 255;
    pixel.green = static_cast<BYTE>(new_green);

    return pixel;
}

Pixel ChangePixelContrastCyan(Pixel pixel, float contrast)
{
    int cyan = (std::min)(static_cast<int>(pixel.green), static_cast<int>(pixel.blue));
    int greenoffset = pixel.green - cyan;
    int blueoffset = pixel.blue - cyan;

    int new_green = static_cast<int>(128 + (cyan - 128) * contrast) + greenoffset;
    if (new_green < 0) new_green = 0; else if (new_green > 255) new_green = 255;
    pixel.green = static_cast<BYTE>(new_green);

    int new_blue = static_cast<int>(128 + (cyan - 128) * contrast) + blueoffset;
    if (new_blue < 0) new_blue = 0; else if (new_blue > 255) new_blue = 255;
    pixel.blue = static_cast<BYTE>(new_blue);

    return pixel;
}

Pixel ChangePixelSaturation(Pixel pixel, float saturation)
{
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    if (pixel.red > average)
    {
        float val = (pixel.red - average) * saturation + average;
        if (val > 255.0f) pixel.red = 255;
        else if (val < 0.0f) pixel.red = 0;
        else pixel.red = static_cast<BYTE>(val);
    }
    if (pixel.green > average)
    {
        float val = (pixel.green - average) * saturation + average;
        if (val > 255.0f) pixel.green = 255;
        else if (val < 0.0f) pixel.green = 0;
        else pixel.green = static_cast<BYTE>(val);
    }
    if (pixel.blue > average)
    {
        float val = (pixel.blue - average) * saturation + average;
        if (val > 255.0f) pixel.blue = 255;
        else if (val < 0.0f) pixel.blue = 0;
        else pixel.blue = static_cast<BYTE>(val);
    }
    return pixel;
}

Pixel ChangePixelSaturationBlue(Pixel pixel, float saturation)
{
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    if ((pixel.blue >= pixel.red) && (pixel.blue >= pixel.green))
    {
        float val = (pixel.blue - average) * saturation + average;
        if (val > 255.0f) pixel.blue = 255;
        else if (val < 0.0f) pixel.blue = 0;
        else pixel.blue = static_cast<BYTE>(val);
    }
    return pixel;
}

Pixel ChangePixelSaturationGreen(Pixel pixel, float saturation)
{
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    if ((pixel.green >= pixel.red) && (pixel.green >= pixel.blue))
    {
        float val = (pixel.green - average) * saturation + average;
        if (val > 255.0f) pixel.green = 255;
        else if (val < 0.0f) pixel.green = 0;
        else pixel.green = static_cast<BYTE>(val);
    }
    return pixel;
}

Pixel ChangePixelSaturationRed(Pixel pixel, float saturation)
{
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    if ((pixel.red >= pixel.blue) && (pixel.red >= pixel.green))
    {
        float val = (pixel.red - average) * saturation + average;
        if (val > 255.0f) pixel.red = 255;
        else if (val < 0.0f) pixel.red = 0;
        else pixel.red = static_cast<BYTE>(val);
    }
    return pixel;
}

Pixel ChangePixelSaturationMagenta(Pixel pixel, float saturation)
{
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    int magenta = (std::min)(static_cast<int>(pixel.red), static_cast<int>(pixel.blue));
    int redOffset = pixel.red - magenta;
    int blueOffset = pixel.blue - magenta;
    if (magenta >= average)
    {
        magenta = static_cast<int>((magenta - average) * saturation + average);
    }
    if ((redOffset + magenta <= 255) && (redOffset + magenta >= 0))
        pixel.red = static_cast<BYTE>(redOffset + magenta);
    else if (redOffset + magenta > 255)
        pixel.red = 255;
    else
        pixel.red = 0;

    if ((blueOffset + magenta <= 255) && (blueOffset + magenta >= 0))
        pixel.blue = static_cast<BYTE>(blueOffset + magenta);
    else if (blueOffset + magenta > 255)
        pixel.blue = 255;
    else
        pixel.blue = 0;

    return pixel;
}

Pixel ChangePixelSaturationYellow(Pixel pixel, float saturation)
{
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    int yellow = (std::min)(static_cast<int>(pixel.red), static_cast<int>(pixel.green));
    int redOffset = pixel.red - yellow;
    int greenOffset = pixel.green - yellow;
    if (yellow >= average)
    {
        yellow = static_cast<int>((yellow - average) * saturation + average);
    }
    if ((redOffset + yellow <= 255) && (redOffset + yellow >= 0))
        pixel.red = static_cast<BYTE>(redOffset + yellow);
    else if (redOffset + yellow > 255)
        pixel.red = 255;
    else
        pixel.red = 0;

    if ((greenOffset + yellow <= 255) && (greenOffset + yellow >= 0))
        pixel.green = static_cast<BYTE>(greenOffset + yellow);
    else if (greenOffset + yellow > 255)
        pixel.green = 255;
    else
        pixel.green = 0;

    return pixel;
}

Pixel ChangePixelSaturationCyan(Pixel pixel, float saturation)
{
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    int cyan = (std::min)(static_cast<int>(pixel.blue), static_cast<int>(pixel.green));
    int greenOffset = pixel.green - cyan;
    int blueOffset = pixel.blue - cyan;
    if (cyan >= average)
    {
        cyan = static_cast<int>((cyan - average) * saturation + average);
    }
    if ((greenOffset + cyan <= 255) && (greenOffset + cyan >= 0))
        pixel.green = static_cast<BYTE>(greenOffset + cyan);
    else if (greenOffset + cyan > 255)
        pixel.green = 255;
    else
        pixel.green = 0;

    if ((blueOffset + cyan <= 255) && (blueOffset + cyan >= 0))
        pixel.blue = static_cast<BYTE>(blueOffset + cyan);
    else if (blueOffset + cyan > 255)
        pixel.blue = 255;
    else
        pixel.blue = 0;

    return pixel;
}

Pixel ChangePixelLuminanceBlue(Pixel pixel, float luminance)
{
    if ((pixel.blue >= pixel.red) && (pixel.blue >= pixel.green))
    {
        int average = (pixel.red + pixel.green + pixel.blue) / 3;
        int newAverage = static_cast<int>(average * luminance);
        int r = (pixel.red - average) + newAverage;
        if (r < 0) r = 0; else if (r > 255) r = 255;
        pixel.red = static_cast<BYTE>(r);

        int g = (pixel.green - average) + newAverage;
        if (g < 0) g = 0; else if (g > 255) g = 255;
        pixel.green = static_cast<BYTE>(g);

        int b = (pixel.blue - average) + newAverage;
        if (b < 0) b = 0; else if (b > 255) b = 255;
        pixel.blue = static_cast<BYTE>(b);
    }
    return pixel;
}

Pixel ChangePixelLuminanceGreen(Pixel pixel, float luminance)
{
    if ((pixel.green >= pixel.red) && (pixel.green >= pixel.blue))
    {
        int average = (pixel.red + pixel.green + pixel.blue) / 3;
        int newAverage = static_cast<int>(average * luminance);
        int r = (pixel.red - average) + newAverage;
        if (r < 0) r = 0; else if (r > 255) r = 255;
        pixel.red = static_cast<BYTE>(r);

        int g = (pixel.green - average) + newAverage;
        if (g < 0) g = 0; else if (g > 255) g = 255;
        pixel.green = static_cast<BYTE>(g);

        int b = (pixel.blue - average) + newAverage;
        if (b < 0) b = 0; else if (b > 255) b = 255;
        pixel.blue = static_cast<BYTE>(b);
    }
    return pixel;
}

Pixel ChangePixelLuminanceRed(Pixel pixel, float luminance)
{
    if ((pixel.red >= pixel.green) && (pixel.red >= pixel.blue))
    {
        int average = (pixel.red + pixel.green + pixel.blue) / 3;
        int newAverage = static_cast<int>(average * luminance);
        int r = (pixel.red - average) + newAverage;
        if (r < 0) r = 0; else if (r > 255) r = 255;
        pixel.red = static_cast<BYTE>(r);

        int g = (pixel.green - average) + newAverage;
        if (g < 0) g = 0; else if (g > 255) g = 255;
        pixel.green = static_cast<BYTE>(g);

        int b = (pixel.blue - average) + newAverage;
        if (b < 0) b = 0; else if (b > 255) b = 255;
        pixel.blue = static_cast<BYTE>(b);
    }
    return pixel;
}

Pixel ChangePixelLuminanceMagenta(Pixel pixel, float luminance)
{
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    int magenta = (std::min)(static_cast<int>(pixel.red), static_cast<int>(pixel.blue));
    if (magenta > average)
    {
        int newAverage = static_cast<int>(average * luminance);
        int r = (pixel.red - average) + newAverage;
        if (r < 0) r = 0; else if (r > 255) r = 255;
        pixel.red = static_cast<BYTE>(r);

        int g = (pixel.green - average) + newAverage;
        if (g < 0) g = 0; else if (g > 255) g = 255;
        pixel.green = static_cast<BYTE>(g);

        int b = (pixel.blue - average) + newAverage;
        if (b < 0) b = 0; else if (b > 255) b = 255;
        pixel.blue = static_cast<BYTE>(b);
    }
    return pixel;
}

Pixel ChangePixelLuminanceYellow(Pixel pixel, float luminance)
{
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    int yellow = (std::min)(static_cast<int>(pixel.red), static_cast<int>(pixel.green));
    if (yellow > average)
    {
        int newAverage = static_cast<int>(average * luminance);
        int r = (pixel.red - average) + newAverage;
        if (r < 0) r = 0; else if (r > 255) r = 255;
        pixel.red = static_cast<BYTE>(r);

        int g = (pixel.green - average) + newAverage;
        if (g < 0) g = 0; else if (g > 255) g = 255;
        pixel.green = static_cast<BYTE>(g);

        int b = (pixel.blue - average) + newAverage;
        if (b < 0) b = 0; else if (b > 255) b = 255;
        pixel.blue = static_cast<BYTE>(b);
    }
    return pixel;
}

Pixel ChangePixelLuminanceCyan(Pixel pixel, float luminance)
{
    int average = (pixel.red + pixel.green + pixel.blue) / 3;
    int cyan = (std::min)(static_cast<int>(pixel.green), static_cast<int>(pixel.blue));
    if (cyan > average)
    {
        int newAverage = static_cast<int>(average * luminance);
        int r = (pixel.red - average) + newAverage;
        if (r < 0) r = 0; else if (r > 255) r = 255;
        pixel.red = static_cast<BYTE>(r);

        int g = (pixel.green - average) + newAverage;
        if (g < 0) g = 0; else if (g > 255) g = 255;
        pixel.green = static_cast<BYTE>(g);

        int b = (pixel.blue - average) + newAverage;
        if (b < 0) b = 0; else if (b > 255) b = 255;
        pixel.blue = static_cast<BYTE>(b);
    }
    return pixel;
}