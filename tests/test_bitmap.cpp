#include <gtest/gtest.h>
#include "bitmap.h"

namespace
{
    // Helper to construct a test Bitmap::File with specified dimensions and bit depth
    Bitmap::File CreateTestBitmap(int width, int height, int bitCount, Pixel fillPixel = {0, 0, 0, 255})
    {
        Bitmap::File bmp;
        bmp.bitmapInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmp.bitmapInfoHeader.biWidth = width;
        bmp.bitmapInfoHeader.biHeight = height;
        bmp.bitmapInfoHeader.biPlanes = 1;
        bmp.bitmapInfoHeader.biBitCount = static_cast<WORD>(bitCount);
        bmp.bitmapInfoHeader.biCompression = 0; // BI_RGB
        
        int bytesPerPixel = bitCount / 8;
        int imageSize = width * height * bytesPerPixel;
        bmp.bitmapInfoHeader.biSizeImage = imageSize;
        bmp.bitmapData.resize(imageSize);

        bmp.bitmapFileHeader.bfType = 0x4D42; // 'BM'
        bmp.bitmapFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        bmp.bitmapFileHeader.bfSize = bmp.bitmapFileHeader.bfOffBits + imageSize;
        bmp.bitmapFileHeader.bfReserved1 = 0;
        bmp.bitmapFileHeader.bfReserved2 = 0;

        int k = 0;
        for (int i = 0; i < height; ++i)
        {
            for (int j = 0; j < width; ++j)
            {
                bmp.bitmapData[k] = fillPixel.blue;
                bmp.bitmapData[k + 1] = fillPixel.green;
                bmp.bitmapData[k + 2] = fillPixel.red;
                if (bytesPerPixel == 4)
                {
                    bmp.bitmapData[k + 3] = fillPixel.alpha;
                }
                k += bytesPerPixel;
            }
        }
        bmp.SetValid();
        return bmp;
    }
}

// ============================================================================
// Conversion Tests: Matrix <-> Bitmap
// ============================================================================

TEST(BitmapConversionTest, CreateMatrixFromBitmap32Bit)
{
    Pixel p{10, 20, 30, 255};
    Bitmap::File bmp = CreateTestBitmap(3, 2, 32, p);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(bmp);

    EXPECT_EQ(mat.rows(), 2);
    EXPECT_EQ(mat.cols(), 3);
    for (size_t r = 0; r < mat.rows(); ++r)
    {
        for (size_t c = 0; c < mat.cols(); ++c)
        {
            EXPECT_EQ(mat[r][c].blue, 10);
            EXPECT_EQ(mat[r][c].green, 20);
            EXPECT_EQ(mat[r][c].red, 30);
            EXPECT_EQ(mat[r][c].alpha, 255);
        }
    }
}

TEST(BitmapConversionTest, CreateMatrixFromBitmap24Bit)
{
    Pixel p{50, 100, 150, 0};
    Bitmap::File bmp = CreateTestBitmap(2, 2, 24, p);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(bmp);

    EXPECT_EQ(mat.rows(), 2);
    EXPECT_EQ(mat.cols(), 2);
    EXPECT_EQ(mat[0][0].blue, 50);
    EXPECT_EQ(mat[0][0].green, 100);
    EXPECT_EQ(mat[0][0].red, 150);
    EXPECT_EQ(mat[0][0].alpha, 0);
}

TEST(BitmapConversionTest, CreateBitmapFromMatrix)
{
    Matrix::Matrix<Pixel> mat(2, 2);
    mat[0][0] = Pixel{1, 2, 3, 255};
    mat[0][1] = Pixel{4, 5, 6, 255};
    mat[1][0] = Pixel{7, 8, 9, 255};
    mat[1][1] = Pixel{10, 11, 12, 255};

    Bitmap::File bmp = CreateBitmapFromMatrix(mat);

    EXPECT_TRUE(bmp.IsValid());
    EXPECT_EQ(bmp.bitmapInfoHeader.biWidth, 2);
    EXPECT_EQ(bmp.bitmapInfoHeader.biHeight, 2);
    EXPECT_EQ(bmp.bitmapInfoHeader.biBitCount, 32);

    Matrix::Matrix<Pixel> roundTrip = CreateMatrixFromBitmap(bmp);
    EXPECT_EQ(roundTrip[0][0], mat[0][0]);
    EXPECT_EQ(roundTrip[0][1], mat[0][1]);
    EXPECT_EQ(roundTrip[1][0], mat[1][0]);
    EXPECT_EQ(roundTrip[1][1], mat[1][1]);
}

// ============================================================================
// Geometric Transformation Tests
// ============================================================================

TEST(BitmapGeometryTest, ShrinkImage)
{
    Matrix::Matrix<Pixel> mat(4, 4);
    for (size_t r = 0; r < 4; ++r)
    {
        for (size_t c = 0; c < 4; ++c)
        {
            mat[r][c] = Pixel{100, 100, 100, 255};
        }
    }
    Bitmap::File bmp = CreateBitmapFromMatrix(mat);
    Bitmap::File shrunken = ShrinkImage(bmp, 2);

    EXPECT_EQ(shrunken.bitmapInfoHeader.biWidth, 2);
    EXPECT_EQ(shrunken.bitmapInfoHeader.biHeight, 2);

    Matrix::Matrix<Pixel> resMat = CreateMatrixFromBitmap(shrunken);
    EXPECT_EQ(resMat[0][0].red, 100);
    EXPECT_EQ(resMat[0][0].green, 100);
    EXPECT_EQ(resMat[0][0].blue, 100);
}

TEST(BitmapGeometryTest, RotateImageClockwise)
{
    // Create 2 rows x 3 cols
    // [ (1,0,0), (2,0,0), (3,0,0) ]
    // [ (4,0,0), (5,0,0), (6,0,0) ]
    Matrix::Matrix<Pixel> mat(2, 3);
    BYTE val = 1;
    for (size_t r = 0; r < 2; ++r)
    {
        for (size_t c = 0; c < 3; ++c)
        {
            mat[r][c] = Pixel{0, 0, val++, 255};
        }
    }

    Bitmap::File bmp = CreateBitmapFromMatrix(mat);
    Bitmap::File rotated = RotateImageClockwise(bmp);

    EXPECT_EQ(rotated.bitmapInfoHeader.biWidth, 2);
    EXPECT_EQ(rotated.bitmapInfoHeader.biHeight, 3);

    Matrix::Matrix<Pixel> res = CreateMatrixFromBitmap(rotated);
    // After 90 deg clockwise:
    // (0,0) becomes (0, 1) = 4
    // (0,1) becomes (0, 0) = 1 in original row 0?
    // Formula in code: rotatedMatrix[rows - j - 1][i] = imageMatrix[i][j]
    EXPECT_EQ(res[3 - 0 - 1][0].red, 1);
    EXPECT_EQ(res[3 - 2 - 1][1].red, 6);
}

TEST(BitmapGeometryTest, RotateImageCounterClockwise)
{
    Matrix::Matrix<Pixel> mat(2, 3);
    BYTE val = 1;
    for (size_t r = 0; r < 2; ++r)
    {
        for (size_t c = 0; c < 3; ++c)
        {
            mat[r][c] = Pixel{0, 0, val++, 255};
        }
    }

    Bitmap::File bmp = CreateBitmapFromMatrix(mat);
    Bitmap::File rotated = RotateImageCounterClockwise(bmp);

    EXPECT_EQ(rotated.bitmapInfoHeader.biWidth, 2);
    EXPECT_EQ(rotated.bitmapInfoHeader.biHeight, 3);

    Matrix::Matrix<Pixel> res = CreateMatrixFromBitmap(rotated);
    // Formula in code: rotatedMatrix[j][cols - i - 1] = imageMatrix[i][j]
    EXPECT_EQ(res[0][2 - 0 - 1].red, 1);
    EXPECT_EQ(res[2][2 - 1 - 1].red, 6);
}

TEST(BitmapGeometryTest, MirrorImage)
{
    Matrix::Matrix<Pixel> mat(1, 2);
    mat[0][0] = Pixel{10, 0, 0, 255};
    mat[0][1] = Pixel{20, 0, 0, 255};

    Bitmap::File bmp = CreateBitmapFromMatrix(mat);
    Bitmap::File mirrored = MirrorImage(bmp);

    Matrix::Matrix<Pixel> res = CreateMatrixFromBitmap(mirrored);
    EXPECT_EQ(res[0][0].blue, 20);
    EXPECT_EQ(res[0][1].blue, 10);
}

TEST(BitmapGeometryTest, FlipImage)
{
    Matrix::Matrix<Pixel> mat(2, 1);
    mat[0][0] = Pixel{10, 0, 0, 255};
    mat[1][0] = Pixel{20, 0, 0, 255};

    Bitmap::File bmp = CreateBitmapFromMatrix(mat);
    Bitmap::File flipped = FlipImage(bmp);

    Matrix::Matrix<Pixel> res = CreateMatrixFromBitmap(flipped);
    EXPECT_EQ(res[0][0].blue, 20);
    EXPECT_EQ(res[1][0].blue, 10);
}

// ============================================================================
// Pixel and Image Greyscale & Brightness Tests
// ============================================================================

TEST(BitmapFilterTest, GreyScalePixel)
{
    Pixel p{30, 60, 90, 255};
    Pixel grey = GreyScalePixel(p);
    EXPECT_EQ(grey.red, 60);
    EXPECT_EQ(grey.green, 60);
    EXPECT_EQ(grey.blue, 60);
    EXPECT_EQ(grey.alpha, 255);
}

TEST(BitmapFilterTest, GreyscaleImage)
{
    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, Pixel{30, 60, 90, 255});
    Bitmap::File greyBmp = GreyscaleImage(bmp);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(greyBmp);
    EXPECT_EQ(mat[0][0].red, 60);
    EXPECT_EQ(mat[0][0].green, 60);
    EXPECT_EQ(mat[0][0].blue, 60);
}

TEST(BitmapFilterTest, ChangePixelBrightness)
{
    Pixel p{100, 100, 100, 255};
    Pixel brighter = ChangePixelBrightness(p, 1.5f);
    EXPECT_EQ(brighter.red, 150);
    EXPECT_EQ(brighter.green, 150);
    EXPECT_EQ(brighter.blue, 150);

    // Clamping test
    Pixel clamped = ChangePixelBrightness(p, 3.0f);
    EXPECT_EQ(clamped.red, 255);
}

TEST(BitmapFilterTest, ChangeImageBrightness)
{
    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, Pixel{100, 100, 100, 255});
    Bitmap::File brightBmp = ChangeImageBrightness(bmp, 1.2f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(brightBmp);
    EXPECT_EQ(mat[0][0].red, 120);
}

// ============================================================================
// Global Contrast & Saturation Tests
// ============================================================================

TEST(BitmapFilterTest, ChangePixelContrast)
{
    Pixel p{148, 148, 148, 255}; // 128 + 20
    Pixel res = ChangePixelContrast(p, 2.0f);
    // 128 + 20 * 2 = 168
    EXPECT_EQ(res.red, 168);
    EXPECT_EQ(res.green, 168);
    EXPECT_EQ(res.blue, 168);
}

TEST(BitmapFilterTest, ChangeImageContrast)
{
    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, Pixel{148, 148, 148, 255});
    Bitmap::File resBmp = ChangeImageContrast(bmp, 2.0f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(resBmp);
    EXPECT_EQ(mat[0][0].red, 168);
}

TEST(BitmapFilterTest, ChangePixelSaturation)
{
    // Red > average (60): average = (120+30+30)/3 = 60
    Pixel p{30, 30, 120, 255}; // blue=30, green=30, red=120
    Pixel res = ChangePixelSaturation(p, 1.5f);
    // red: (120 - 60) * 1.5 + 60 = 150
    EXPECT_EQ(res.red, 150);
    EXPECT_EQ(res.green, 30);
    EXPECT_EQ(res.blue, 30);
}

TEST(BitmapFilterTest, ChangeImageSaturation)
{
    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, Pixel{30, 30, 120, 255});
    Bitmap::File resBmp = ChangeImageSaturation(bmp, 1.5f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(resBmp);
    EXPECT_EQ(mat[0][0].red, 150);
}

// ============================================================================
// Color-Specific Contrast Tests (Pixel & Image)
// ============================================================================

TEST(BitmapFilterTest, ContrastRed)
{
    Pixel p{100, 100, 148, 255};
    Pixel res = ChangePixelContrastRed(p, 2.0f);
    EXPECT_EQ(res.red, 168);
    EXPECT_EQ(res.green, 100);
    EXPECT_EQ(res.blue, 100);

    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, p);
    Bitmap::File resBmp = ChangeImageContrastRed(bmp, 2.0f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(resBmp);
    EXPECT_EQ(mat[0][0].red, 168);
}

TEST(BitmapFilterTest, ContrastGreen)
{
    Pixel p{100, 148, 100, 255};
    Pixel res = ChangePixelContrastGreen(p, 2.0f);
    EXPECT_EQ(res.green, 168);
    EXPECT_EQ(res.red, 100);
    EXPECT_EQ(res.blue, 100);

    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, p);
    Bitmap::File resBmp = ChangeImageContrastGreen(bmp, 2.0f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(resBmp);
    EXPECT_EQ(mat[0][0].green, 168);
}

TEST(BitmapFilterTest, ContrastBlue)
{
    Pixel p{148, 100, 100, 255};
    Pixel res = ChangePixelContrastBlue(p, 2.0f);
    EXPECT_EQ(res.blue, 168);
    EXPECT_EQ(res.red, 100);
    EXPECT_EQ(res.green, 100);

    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, p);
    Bitmap::File resBmp = ChangeImageContrastBlue(bmp, 2.0f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(resBmp);
    EXPECT_EQ(mat[0][0].blue, 168);
}

TEST(BitmapFilterTest, ContrastMagenta)
{
    // Magenta is min(red, blue)
    Pixel p{148, 50, 148, 255};
    Pixel res = ChangePixelContrastMagenta(p, 2.0f);
    EXPECT_EQ(res.red, 168);
    EXPECT_EQ(res.blue, 168);
    EXPECT_EQ(res.green, 50);

    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, p);
    Bitmap::File resBmp = ChangeImageContrastMagenta(bmp, 2.0f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(resBmp);
    EXPECT_EQ(mat[0][0].red, 168);
    EXPECT_EQ(mat[0][0].blue, 168);
}

TEST(BitmapFilterTest, ContrastYellow)
{
    // Yellow is min(red, green)
    Pixel p{50, 148, 148, 255};
    Pixel res = ChangePixelContrastYellow(p, 2.0f);
    EXPECT_EQ(res.red, 168);
    EXPECT_EQ(res.green, 168);
    EXPECT_EQ(res.blue, 50);

    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, p);
    Bitmap::File resBmp = ChangeImageContrastYellow(bmp, 2.0f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(resBmp);
    EXPECT_EQ(mat[0][0].red, 168);
    EXPECT_EQ(mat[0][0].green, 168);
}

TEST(BitmapFilterTest, ContrastCyan)
{
    // Cyan is min(green, blue)
    Pixel p{148, 148, 50, 255};
    Pixel res = ChangePixelContrastCyan(p, 2.0f);
    EXPECT_EQ(res.green, 168);
    EXPECT_EQ(res.blue, 168);
    EXPECT_EQ(res.red, 50);

    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, p);
    Bitmap::File resBmp = ChangeImageContrastCyan(bmp, 2.0f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(resBmp);
    EXPECT_EQ(mat[0][0].green, 168);
    EXPECT_EQ(mat[0][0].blue, 168);
}

// ============================================================================
// Color-Specific Saturation Tests (Pixel & Image)
// ============================================================================

TEST(BitmapFilterTest, SaturationRed)
{
    // red dominant: red=150, green=30, blue=30 -> avg=70
    Pixel p{30, 30, 150, 255};
    Pixel res = ChangePixelSaturationRed(p, 1.5f);
    // (150 - 70) * 1.5 + 70 = 190
    EXPECT_EQ(res.red, 190);

    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, p);
    Bitmap::File resBmp = ChangeImageSaturationRed(bmp, 1.5f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(resBmp);
    EXPECT_EQ(mat[0][0].red, 190);
}

TEST(BitmapFilterTest, SaturationGreen)
{
    Pixel p{30, 150, 30, 255};
    Pixel res = ChangePixelSaturationGreen(p, 1.5f);
    EXPECT_EQ(res.green, 190);

    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, p);
    Bitmap::File resBmp = ChangeImageSaturationGreen(bmp, 1.5f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(resBmp);
    EXPECT_EQ(mat[0][0].green, 190);
}

TEST(BitmapFilterTest, SaturationBlue)
{
    Pixel p{150, 30, 30, 255};
    Pixel res = ChangePixelSaturationBlue(p, 1.5f);
    EXPECT_EQ(res.blue, 190);

    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, p);
    Bitmap::File resBmp = ChangeImageSaturationBlue(bmp, 1.5f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(resBmp);
    EXPECT_EQ(mat[0][0].blue, 190);
}

TEST(BitmapFilterTest, SaturationMagenta)
{
    // red=120, green=30, blue=120 -> avg = 90. magenta = 120.
    Pixel p{120, 30, 120, 255};
    Pixel res = ChangePixelSaturationMagenta(p, 2.0f);
    // (120 - 90) * 2 + 90 = 150
    EXPECT_EQ(res.red, 150);
    EXPECT_EQ(res.blue, 150);

    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, p);
    Bitmap::File resBmp = ChangeImageSaturationMagenta(bmp, 2.0f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(resBmp);
    EXPECT_EQ(mat[0][0].red, 150);
    EXPECT_EQ(mat[0][0].blue, 150);
}

TEST(BitmapFilterTest, SaturationYellow)
{
    Pixel p{30, 120, 120, 255};
    Pixel res = ChangePixelSaturationYellow(p, 2.0f);
    EXPECT_EQ(res.red, 150);
    EXPECT_EQ(res.green, 150);

    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, p);
    Bitmap::File resBmp = ChangeImageSaturationYellow(bmp, 2.0f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(resBmp);
    EXPECT_EQ(mat[0][0].red, 150);
    EXPECT_EQ(mat[0][0].green, 150);
}

TEST(BitmapFilterTest, SaturationCyan)
{
    Pixel p{120, 120, 30, 255};
    Pixel res = ChangePixelSaturationCyan(p, 2.0f);
    EXPECT_EQ(res.green, 150);
    EXPECT_EQ(res.blue, 150);

    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, p);
    Bitmap::File resBmp = ChangeImageSaturationCyan(bmp, 2.0f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(resBmp);
    EXPECT_EQ(mat[0][0].green, 150);
    EXPECT_EQ(mat[0][0].blue, 150);
}

// ============================================================================
// Color-Specific Luminance Tests (Pixel & Image)
// ============================================================================

TEST(BitmapFilterTest, LuminanceRed)
{
    // red dominant: red=120, green=30, blue=30 -> avg=60
    Pixel p{30, 30, 120, 255};
    Pixel res = ChangePixelLuminanceRed(p, 1.5f);
    // newAverage = 60 * 1.5 = 90
    // red = (120 - 60) + 90 = 150
    // green = (30 - 60) + 90 = 60
    // blue = (30 - 60) + 90 = 60
    EXPECT_EQ(res.red, 150);
    EXPECT_EQ(res.green, 60);
    EXPECT_EQ(res.blue, 60);

    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, p);
    Bitmap::File resBmp = ChangeImageLuminanceRed(bmp, 1.5f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(resBmp);
    EXPECT_EQ(mat[0][0].red, 150);
}

TEST(BitmapFilterTest, LuminanceGreen)
{
    Pixel p{30, 120, 30, 255};
    Pixel res = ChangePixelLuminanceGreen(p, 1.5f);
    EXPECT_EQ(res.green, 150);
    EXPECT_EQ(res.red, 60);
    EXPECT_EQ(res.blue, 60);

    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, p);
    Bitmap::File resBmp = ChangeImageLuminanceGreen(bmp, 1.5f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(resBmp);
    EXPECT_EQ(mat[0][0].green, 150);
}

TEST(BitmapFilterTest, LuminanceBlue)
{
    Pixel p{120, 30, 30, 255};
    Pixel res = ChangePixelLuminanceBlue(p, 1.5f);
    EXPECT_EQ(res.blue, 150);
    EXPECT_EQ(res.red, 60);
    EXPECT_EQ(res.green, 60);

    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, p);
    Bitmap::File resBmp = ChangeImageLuminanceBlue(bmp, 1.5f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(resBmp);
    EXPECT_EQ(mat[0][0].blue, 150);
}

TEST(BitmapFilterTest, LuminanceMagenta)
{
    Pixel p{100, 20, 100, 255}; // avg = 73, magenta = 100 > 73
    Pixel res = ChangePixelLuminanceMagenta(p, 1.2f);
    EXPECT_GT(res.red, 100);
    EXPECT_GT(res.blue, 100);

    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, p);
    Bitmap::File resBmp = ChangeImageLuminanceMagenta(bmp, 1.2f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(resBmp);
    EXPECT_EQ(mat[0][0].red, res.red);
}

TEST(BitmapFilterTest, LuminanceYellow)
{
    Pixel p{20, 100, 100, 255}; // avg = 73, yellow = 100 > 73
    Pixel res = ChangePixelLuminanceYellow(p, 1.2f);
    EXPECT_GT(res.red, 100);
    EXPECT_GT(res.green, 100);

    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, p);
    Bitmap::File resBmp = ChangeImageLuminanceYellow(bmp, 1.2f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(resBmp);
    EXPECT_EQ(mat[0][0].red, res.red);
}

TEST(BitmapFilterTest, LuminanceCyan)
{
    Pixel p{100, 100, 20, 255}; // avg = 73, cyan = 100 > 73
    Pixel res = ChangePixelLuminanceCyan(p, 1.2f);
    EXPECT_GT(res.green, 100);
    EXPECT_GT(res.blue, 100);

    Bitmap::File bmp = CreateTestBitmap(2, 2, 32, p);
    Bitmap::File resBmp = ChangeImageLuminanceCyan(bmp, 1.2f);
    Matrix::Matrix<Pixel> mat = CreateMatrixFromBitmap(resBmp);
    EXPECT_EQ(mat[0][0].green, res.green);
}

// ============================================================================
// Platform Utilities (ScreenShotWindow)
// ============================================================================

#if defined(_WIN32)
TEST(BitmapPlatformTest, ScreenShotWindowNullHandle)
{
    Bitmap::File bmp = ScreenShotWindow(nullptr);
    EXPECT_FALSE(bmp.IsValid());
}

TEST(BitmapPlatformTest, ScreenShotWindowDesktop)
{
    HWND desktop = GetDesktopWindow();
    if (desktop != nullptr)
    {
        Bitmap::File bmp = ScreenShotWindow(desktop);
        EXPECT_TRUE(bmp.IsValid());
        EXPECT_GT(bmp.bitmapInfoHeader.biWidth, 0);
        EXPECT_GT(bmp.bitmapInfoHeader.biHeight, 0);
    }
}
#endif
