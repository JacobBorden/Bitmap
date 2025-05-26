#ifndef _BITMAP_
#define _BITMAP_

#include "matrix/matrix.h"
#include "bitmapfile/bitmap_file.h"
#include <cmath>     // For mathematical operations like sqrt, pow.
#include <algorithm> // For std::min, std::max.

// Defines a pixel structure with Blue, Green, Red, and Alpha channels.
struct Pixel
{
  BYTE blue;  // Blue channel intensity.
  BYTE green; // Green channel intensity.
  BYTE red;   // Red channel intensity.
  BYTE alpha; // Alpha channel (transparency).
};

// Converts a Bitmap::File object into a Matrix::Matrix<Pixel>.
// Parameters:
//   bitmapFile: The Bitmap::File object to convert.
// Returns:
//   A Matrix::Matrix<Pixel> representing the image.
Matrix::Matrix<Pixel> CreateMatrixFromBitmap(Bitmap::File bitmapFile);

// Captures a screenshot of a specified window.
// Parameters:
//   WindowHandle: Handle to the window to capture.
// Returns:
//   A Bitmap::File object containing the screenshot.
#ifdef _WIN32
Bitmap::File ScreenShotWindow(HWND WindowHandle);
#endif // _WIN32
// Converts a Matrix::Matrix<Pixel> into a Bitmap::File object.
// Parameters:
//   imageMatrix: The Matrix::Matrix<Pixel> to convert.
// Returns:
//   A Bitmap::File object representing the image.
Bitmap::File CreateBitmapFromMatrix(const Matrix::Matrix<Pixel> &imageMatrix);

// Shrinks the image by a given scale factor.
// Parameters:
//   bitmapFile: The image to shrink.
//   scaleFactor: The factor by which to shrink the image (e.g., 2 for half size).
// Returns:
//   A new Bitmap::File object with the shrunken image.
Bitmap::File ShrinkImage(Bitmap::File bitmapFile, int scaleFactor);

// Rotates the image 90 degrees counter-clockwise.
// Parameters:
//   bitmapFile: The image to rotate.
// Returns:
//   A new Bitmap::File object with the rotated image.
Bitmap::File RotateImageCounterClockwise(Bitmap::File bitmapFile);

// Rotates the image 90 degrees clockwise.
// Parameters:
//   bitmapFile: The image to rotate.
// Returns:
//   A new Bitmap::File object with the rotated image.
Bitmap::File RotateImageClockwise(Bitmap::File bitmapFile);

// Mirrors the image horizontally.
// Parameters:
//   bitmapFile: The image to mirror.
// Returns:
//   A new Bitmap::File object with the mirrored image.
Bitmap::File MirrorImage(Bitmap::File bitmapFile);

// Flips the image vertically.
// Parameters:
//   bitmapFile: The image to flip.
// Returns:
//   A new Bitmap::File object with the flipped image.
Bitmap::File FlipImage(Bitmap::File bitmapFile);

// Converts the image to greyscale.
// Parameters:
//   bitmapFile: The image to convert.
// Returns:
//   A new Bitmap::File object with the greyscale image.
Bitmap::File GreyscaleImage(Bitmap::File bitmapFile);

// Changes the overall brightness of the image.
// Parameters:
//   bitmapFile: The image to modify.
//   brightness: The brightness factor (e.g., 1.0 for no change, >1.0 for brighter, <1.0 for darker).
// Returns:
//   A new Bitmap::File object with adjusted brightness.
Bitmap::File ChangeImageBrightness(Bitmap::File bitmapFile, float brightness);

// Changes the overall contrast of the image.
// Parameters:
//   bitmapFile: The image to modify.
//   contrast: The contrast factor (e.g., 1.0 for no change).
// Returns:
//   A new Bitmap::File object with adjusted contrast.
Bitmap::File ChangeImageContrast(Bitmap::File bitmapFile, float contrast);

// Changes the overall saturation of the image.
// Parameters:
//   bitmapFile: The image to modify.
//   saturation: The saturation factor (e.g., 1.0 for no change, >1.0 for more saturated, <1.0 for less saturated).
// Returns:
//   A new Bitmap::File object with adjusted saturation.
Bitmap::File ChangeImageSaturation(Bitmap::File bitmapFile, float saturation);

// Changes the saturation of the blue channel in the image.
// Parameters:
//   bitmapFile: The image to modify.
//   saturation: The saturation factor for the blue channel.
// Returns:
//   A new Bitmap::File object with adjusted blue channel saturation.
Bitmap::File ChangeImageSaturationBlue(Bitmap::File bitmapFile, float saturation);

// Changes the saturation of the green channel in the image.
// Parameters:
//   bitmapFile: The image to modify.
//   saturation: The saturation factor for the green channel.
// Returns:
//   A new Bitmap::File object with adjusted green channel saturation.
Bitmap::File ChangeImageSaturationGreen(Bitmap::File bitmapFile, float saturation);

// Changes the saturation of the red channel in the image.
// Parameters:
//   bitmapFile: The image to modify.
//   saturation: The saturation factor for the red channel.
// Returns:
//   A new Bitmap::File object with adjusted red channel saturation.
Bitmap::File ChangeImageSaturationRed(Bitmap::File bitmapFile, float saturation);

// Changes the saturation of the magenta component (red and blue channels) in the image.
// Parameters:
//   bitmapFile: The image to modify.
//   saturation: The saturation factor for magenta.
// Returns:
//   A new Bitmap::File object with adjusted magenta saturation.
Bitmap::File ChangeImageSaturationMagenta(Bitmap::File bitmapFile, float saturation);

// Changes the saturation of the yellow component (red and green channels) in the image.
// Parameters:
//   bitmapFile: The image to modify.
//   saturation: The saturation factor for yellow.
// Returns:
//   A new Bitmap::File object with adjusted yellow saturation.
Bitmap::File ChangeImageSaturationYellow(Bitmap::File bitmapFile, float saturation);

// Changes the saturation of the cyan component (green and blue channels) in the image.
// Parameters:
//   bitmapFile: The image to modify.
//   saturation: The saturation factor for cyan.
// Returns:
//   A new Bitmap::File object with adjusted cyan saturation.
Bitmap::File ChangeImageSaturationCyan(Bitmap::File bitmapFile, float saturation);

// Changes the luminance of the blue channel in the image.
// Parameters:
//   bitmapFile: The image to modify.
//   luminance: The luminance factor for the blue channel. (Note: Parameter name in .cpp is 'saturation' in original code, kept for consistency for now)
// Returns:
//   A new Bitmap::File object with adjusted blue channel luminance.
Bitmap::File ChangeImageLuminanceBlue(Bitmap::File bitmapFile, float luminance);

// Changes the luminance of the green channel in the image.
// Parameters:
//   bitmapFile: The image to modify.
//   luminance: The luminance factor for the green channel. (Note: Parameter name in .cpp is 'saturation' in original code, kept for consistency for now)
// Returns:
//   A new Bitmap::File object with adjusted green channel luminance.
Bitmap::File ChangeImageLuminanceGreen(Bitmap::File bitmapFile, float luminance);

// Changes the luminance of the red channel in the image.
// Parameters:
//   bitmapFile: The image to modify.
//   luminance: The luminance factor for the red channel. (Note: Parameter name in .cpp is 'saturation' in original code, kept for consistency for now)
// Returns:
//   A new Bitmap::File object with adjusted red channel luminance.
Bitmap::File ChangeImageLuminanceRed(Bitmap::File bitmapFile, float luminance);

// Changes the luminance of the magenta component (red and blue channels) in the image.
// Parameters:
//   bitmapFile: The image to modify.
//   luminance: The luminance factor for magenta. (Note: Parameter name in .cpp is 'saturation' in original code, kept for consistency for now)
// Returns:
//   A new Bitmap::File object with adjusted magenta luminance.
Bitmap::File ChangeImageLuminanceMagenta(Bitmap::File bitmapFile, float luminance);

// Changes the luminance of the yellow component (red and green channels) in the image.
// Parameters:
//   bitmapFile: The image to modify.
//   luminance: The luminance factor for yellow. (Note: Parameter name in .cpp is 'saturation' in original code, kept for consistency for now)
// Returns:
//   A new Bitmap::File object with adjusted yellow luminance.
Bitmap::File ChangeImageLuminanceYellow(Bitmap::File bitmapFile, float luminance);

// Changes the luminance of the cyan component (green and blue channels) in the image.
// Parameters:
//   bitmapFile: The image to modify.
//   luminance: The luminance factor for cyan. (Note: Parameter name in .cpp is 'saturation' in original code, kept for consistency for now)
// Returns:
//   A new Bitmap::File object with adjusted cyan luminance.
Bitmap::File ChangeImageLuminanceCyan(Bitmap::File bitmapFile, float luminance);

// Converts a single pixel to greyscale.
// Parameters:
//   pixel: The input pixel.
// Returns:
//   The greyscale equivalent of the input pixel.
Pixel GreyScalePixel(Pixel pixel);

// Changes the brightness of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   brightness: The brightness factor.
// Returns:
//   The pixel with adjusted brightness.
Pixel ChangePixelBrightness(Pixel pixel, float brightness); // Note: original param name 'brightnessl' corrected to 'brightness'

// Changes the contrast of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   contrast: The contrast factor.
// Returns:
//   The pixel with adjusted contrast.
Pixel ChangePixelContrast(Pixel pixel, float contrast);

// Changes the contrast of the red channel of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   contrast: The contrast factor.
// Returns:
//   The pixel with adjusted red channel contrast.
Pixel ChangePixelContrastRed(Pixel pixel, float contrast);

// Changes the contrast of the green channel of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   contrast: The contrast factor.
// Returns:
//   The pixel with adjusted green channel contrast.
Pixel ChangePixelContrastGreen(Pixel pixel, float contrast);

// Changes the contrast of the blue channel of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   contrast: The contrast factor.
// Returns:
//   The pixel with adjusted blue channel contrast.
Pixel ChangePixelContrastBlue(Pixel pixel, float contrast);

// Changes the contrast of the magenta component (red and blue channels) of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   contrast: The contrast factor.
// Returns:
//   The pixel with adjusted magenta contrast.
Pixel ChangePixelContrastMagenta(Pixel pixel, float contrast);

// Changes the contrast of the yellow component (red and green channels) of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   contrast: The contrast factor.
// Returns:
//   The pixel with adjusted yellow contrast.
Pixel ChangePixelContrastYellow(Pixel pixel, float contrast);

// Changes the contrast of the cyan component (green and blue channels) of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   contrast: The contrast factor.
// Returns:
//   The pixel with adjusted cyan contrast.
Pixel ChangePixelContrastCyan(Pixel pixel, float contrast);

// Changes the saturation of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   saturation: The saturation factor.
// Returns:
//   The pixel with adjusted saturation.
Pixel ChangePixelSaturation(Pixel pixel, float saturation);

// Changes the saturation of the blue channel of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   saturation: The saturation factor.
// Returns:
//   The pixel with adjusted blue channel saturation.
Pixel ChangePixelSaturationBlue(Pixel pixel, float saturation);

// Changes the saturation of the green channel of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   saturation: The saturation factor.
// Returns:
//   The pixel with adjusted green channel saturation.
Pixel ChangePixelSaturationGreen(Pixel pixel, float saturation);

// Changes the saturation of the red channel of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   saturation: The saturation factor.
// Returns:
//   The pixel with adjusted red channel saturation.
Pixel ChangePixelSaturationRed(Pixel pixel, float saturation);

// Changes the saturation of the magenta component (red and blue channels) of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   saturation: The saturation factor.
// Returns:
//   The pixel with adjusted magenta saturation.
Pixel ChangePixelSaturationMagenta(Pixel pixel, float saturation);

// Changes the saturation of the yellow component (red and green channels) of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   saturation: The saturation factor.
// Returns:
//   The pixel with adjusted yellow saturation.
Pixel ChangePixelSaturationYellow(Pixel pixel, float saturation);

// Changes the saturation of the cyan component (green and blue channels) of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   saturation: The saturation factor.
// Returns:
//   The pixel with adjusted cyan saturation.
Pixel ChangePixelSaturationCyan(Pixel pixel, float saturation);

// Changes the luminance of the blue channel of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   luminance: The luminance factor.
// Returns:
//   The pixel with adjusted blue channel luminance.
Pixel ChangePixelLuminanceBlue(Pixel pixel, float luminance);

// Changes the luminance of the green channel of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   luminance: The luminance factor.
// Returns:
//   The pixel with adjusted green channel luminance.
Pixel ChangePixelLuminanceGreen(Pixel pixel, float luminance);

// Changes the luminance of the red channel of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   luminance: The luminance factor.
// Returns:
//   The pixel with adjusted red channel luminance.
Pixel ChangePixelLuminanceRed(Pixel pixel, float luminance);

// Changes the luminance of the magenta component (red and blue channels) of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   luminance: The luminance factor.
// Returns:
//   The pixel with adjusted magenta luminance.
Pixel ChangePixelLuminanceMagenta(Pixel pixel, float luminance);

// Changes the luminance of the yellow component (red and green channels) of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   luminance: The luminance factor.
// Returns:
//   The pixel with adjusted yellow luminance.
Pixel ChangePixelLuminanceYellow(Pixel pixel, float luminance);

// Changes the luminance of the cyan component (green and blue channels) of a single pixel.
// Parameters:
//   pixel: The input pixel.
//   luminance: The luminance factor.
// Returns:
//   The pixel with adjusted cyan luminance.
Pixel ChangePixelLuminanceCyan(Pixel pixel, float luminance);

// ---- New image manipulation functions ----

// Inverts the colors of the image.
// Parameters:
//   bitmapFile: The image to invert.
// Returns:
//   A new Bitmap::File object with inverted colors.
Bitmap::File InvertImageColors(Bitmap::File bitmapFile);

// Applies a sepia tone to the image.
// Parameters:
//   bitmapFile: The image to apply sepia tone to.
// Returns:
//   A new Bitmap::File object with sepia tone applied.
Bitmap::File ApplySepiaTone(Bitmap::File bitmapFile);

// Applies a box blur to the image.
// Parameters:
//   bitmapFile: The image to blur.
//   blurRadius: The radius of the blur box (e.g., 1 for a 3x3 box). Defaults to 1.
// Returns:
//   A new Bitmap::File object with the box blur applied.
Bitmap::File ApplyBoxBlur(Bitmap::File bitmapFile, int blurRadius = 1);

// ---- New pixel manipulation functions (helpers for the above) ----

// Inverts the color of a single pixel (Red, Green, Blue channels). Alpha is unchanged.
// Parameters:
//   pixel: The input pixel.
// Returns:
//   The pixel with inverted RGB colors.
Pixel InvertPixelColor(Pixel pixel);

// Applies sepia tone to a single pixel. Alpha is unchanged.
// Parameters:
//   pixel: The input pixel.
// Returns:
//   The pixel with sepia tone applied.
Pixel ApplySepiaToPixel(Pixel pixel);

// Note: Box blur is applied over a region within ApplyBoxBlur,
// so it does not have a direct single-pixel helper function here.
#endif