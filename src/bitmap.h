#ifndef _BITMAP_
#define _BITMAP_

#include "../dependencies/matrix/matrix.h"
#include "../dependencies/bitmapfile/src/bitmap_file.h"
#include <cmath>
#include <algorithm>

struct Pixel
{
  BYTE blue;
  BYTE green;
  BYTE red;
  BYTE alpha;
};

Matrix::Matrix<Pixel> CreateMatrixFromBitmap(Bitmap::File bitmapFile);

Bitmap::File ScreenShotWindow(HWND WindowHandle);
Bitmap::File CreateBitmapFromMatrix(Matrix::Matrix<Pixel> imageMatix);
Bitmap::File ShrinkImage(Bitmap::File bitmapFile, int scaleFactor);
Bitmap::File RotateImageCounterClockwise(Bitmap::File bitmapFile);
Bitmap::File RotateImageClockwise(Bitmap::File bitmapFile);
Bitmap::File MirrorImage(Bitmap::File bitmapFile);
Bitmap::File FlipImage(Bitmap::File bitmapFile);
Bitmap::File GreyscaleImage(Bitmap::File bitmapFile);
Bitmap::File ChangeImageBrightness(Bitmap::File bitmapFile, float brightness);
Bitmap::File ChangeImageContrast(Bitmap::File bitmapFile, float contrast);
Bitmap::File ChangeImageSaturation(Bitmap::File bitmapFile, float saturation);
Bitmap::File ChangeImageSaturationBlue(Bitmap::File bitmapFile, float saturation);
Bitmap::File ChangeImageSaturationGreen(Bitmap::File bitmapFile, float saturation);
Bitmap::File ChangeImageSaturationRed(Bitmap::File bitmapFile, float saturation);
Bitmap::File ChangeImageSaturationMagenta(Bitmap::File bitmapFile, float saturation);
Bitmap::File ChangeImageSaturationYellow(Bitmap::File bitmapFile, float saturation);
Bitmap::File ChangeImageSaturationCyan(Bitmap::File bitmapFile, float saturation);
Bitmap::File ChangeImageLuminanceBlue(Bitmap::File bitmapFile, float saturation);
Bitmap::File ChangeImageLuminanceGreen(Bitmap::File bitmapFile, float saturation);
Bitmap::File ChangeImageLuminanceRed(Bitmap::File bitmapFile, float saturation);
Bitmap::File ChangeImageLuminanceMagenta(Bitmap::File bitmapFile, float saturation);
Bitmap::File ChangeImageLuminanceYellow(Bitmap::File bitmapFile, float saturation);
Bitmap::File ChangeImageLuminanceCyan(Bitmap::File bitmapFile, float saturation);
Pixel GreyScalePixel(Pixel pixel);
Pixel ChangePixelBrightness(Pixel pixel, float brightnessl);
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