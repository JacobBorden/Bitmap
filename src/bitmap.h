#ifndef _BITMAP_
#define _BITMAP_

#if defined(_WIN32)
#include <windows.h>
#endif

#include "matrix.h"
#include "bitmap_file.h"
#include <cmath>
#include <algorithm>
#include <ostream>

struct Pixel
{
    BYTE blue;
    BYTE green;
    BYTE red;
    BYTE alpha;

    bool operator==(const Pixel& other) const
    {
        return blue == other.blue && green == other.green && red == other.red && alpha == other.alpha;
    }

    bool operator!=(const Pixel& other) const
    {
        return !(*this == other);
    }
};

inline std::ostream& operator<<(std::ostream& os, const Pixel& p)
{
    return os << "Pixel(R:" << static_cast<int>(p.red)
              << ", G:" << static_cast<int>(p.green)
              << ", B:" << static_cast<int>(p.blue)
              << ", A:" << static_cast<int>(p.alpha) << ")";
}

Matrix::Matrix<Pixel> CreateMatrixFromBitmap(Bitmap::File bitmapFile);

#if defined(_WIN32)
Bitmap::File ScreenShotWindow(HWND WindowHandle);
#endif

Bitmap::File CreateBitmapFromMatrix(const Matrix::Matrix<Pixel>& imageMatix);
Bitmap::File ShrinkImage(Bitmap::File bitmapFile, int scaleFactor);
Bitmap::File RotateImageCounterClockwise(Bitmap::File bitmapFile);
Bitmap::File RotateImageClockwise(Bitmap::File bitmapFile);
Bitmap::File MirrorImage(Bitmap::File bitmapFile);
Bitmap::File FlipImage(Bitmap::File bitmapFile);
Bitmap::File GreyscaleImage(Bitmap::File bitmapFile);
Bitmap::File ChangeImageBrightness(Bitmap::File bitmapFile, float brightness);
Bitmap::File ChangeImageContrast(Bitmap::File bitmapFile, float contrast);
Bitmap::File ChangeImageContrastRed(Bitmap::File bitmapFile, float contrast);
Bitmap::File ChangeImageContrastGreen(Bitmap::File bitmapFile, float contrast);
Bitmap::File ChangeImageContrastBlue(Bitmap::File bitmapFile, float contrast);
Bitmap::File ChangeImageContrastMagenta(Bitmap::File bitmapFile, float contrast);
Bitmap::File ChangeImageContrastYellow(Bitmap::File bitmapFile, float contrast);
Bitmap::File ChangeImageContrastCyan(Bitmap::File bitmapFile, float contrast);

Bitmap::File ChangeImageSaturation(Bitmap::File bitmapFile, float saturation);
Bitmap::File ChangeImageSaturationBlue(Bitmap::File bitmapFile, float saturation);
Bitmap::File ChangeImageSaturationGreen(Bitmap::File bitmapFile, float saturation);
Bitmap::File ChangeImageSaturationRed(Bitmap::File bitmapFile, float saturation);
Bitmap::File ChangeImageSaturationMagenta(Bitmap::File bitmapFile, float saturation);
Bitmap::File ChangeImageSaturationYellow(Bitmap::File bitmapFile, float saturation);
Bitmap::File ChangeImageSaturationCyan(Bitmap::File bitmapFile, float saturation);

Bitmap::File ChangeImageLuminanceBlue(Bitmap::File bitmapFile, float luminance);
Bitmap::File ChangeImageLuminanceGreen(Bitmap::File bitmapFile, float luminance);
Bitmap::File ChangeImageLuminanceRed(Bitmap::File bitmapFile, float luminance);
Bitmap::File ChangeImageLuminanceMagenta(Bitmap::File bitmapFile, float luminance);
Bitmap::File ChangeImageLuminanceYellow(Bitmap::File bitmapFile, float luminance);
Bitmap::File ChangeImageLuminanceCyan(Bitmap::File bitmapFile, float luminance);

Pixel GreyScalePixel(Pixel pixel);
Pixel ChangePixelBrightness(Pixel pixel, float brightness);
Pixel ChangePixelContrast(Pixel pixel, float contrast);
Pixel ChangePixelContrastRed(Pixel pixel, float contrast);
Pixel ChangePixelContrastGreen(Pixel pixel, float contrast);
Pixel ChangePixelContrastBlue(Pixel pixel, float contrast);
Pixel ChangePixelContrastMagenta(Pixel pixel, float contrast);
Pixel ChangePixelContrastYellow(Pixel pixel, float contrast);
Pixel ChangePixelContrastCyan(Pixel pixel, float contrast);
Pixel ChangePixelSaturation(Pixel pixel, float saturation);
Pixel ChangePixelSaturationBlue(Pixel pixel, float saturation);
Pixel ChangePixelSaturationGreen(Pixel pixel, float saturation);
Pixel ChangePixelSaturationRed(Pixel pixel, float saturation);
Pixel ChangePixelSaturationMagenta(Pixel pixel, float saturation);
Pixel ChangePixelSaturationYellow(Pixel pixel, float saturation);
Pixel ChangePixelSaturationCyan(Pixel pixel, float saturation);
Pixel ChangePixelLuminanceBlue(Pixel pixel, float luminance);
Pixel ChangePixelLuminanceGreen(Pixel pixel, float luminance);
Pixel ChangePixelLuminanceRed(Pixel pixel, float luminance);
Pixel ChangePixelLuminanceMagenta(Pixel pixel, float luminance);
Pixel ChangePixelLuminanceYellow(Pixel pixel, float luminance);
Pixel ChangePixelLuminanceCyan(Pixel pixel, float luminance);

#endif