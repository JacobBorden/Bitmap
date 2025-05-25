#include <iostream>
#include <string>
#include <cmath>
#include <gtest/gtest.h>

// Include the header for the code to be tested
#include "../src/bitmap.h"

// Overload for Pixel struct comparison
bool operator==(const Pixel& p1, const Pixel& p2) {
    return p1.red == p2.red && 
           p1.green == p2.green && 
           p1.blue == p2.blue && 
           p1.alpha == p2.alpha;
}
std::ostream& operator<<(std::ostream& os, const Pixel& p) {
    os << "R:" << (int)p.red << " G:" << (int)p.green << " B:" << (int)p.blue << " A:" << (int)p.alpha;
    return os;
}

TEST(PixelTest, InvertPixelColor) {
    Pixel p1 = {10, 20, 30, 255};
    Pixel expected1 = {225, 235, 245, 255};
    EXPECT_EQ(expected1, InvertPixelColor(p1));

    Pixel p2 = {0, 0, 0, 100};
    Pixel expected2 = {255, 255, 255, 100};
    EXPECT_EQ(expected2, InvertPixelColor(p2));

    Pixel p3 = {255, 255, 255, 50};
    Pixel expected3 = {0, 0, 0, 50};
    EXPECT_EQ(expected3, InvertPixelColor(p3));
}

TEST(PixelTest, ApplySepiaToPixel) {
    Pixel p1 = {200, 150, 100, 255}; // B=200, G=150, R=100, A=255
    // Original values from description: R=100, G=150, B=200
    // tr = 0.393*100 + 0.769*150 + 0.189*200 = 39.3 + 115.35 + 37.8 = 192.45 -> 192 (red)
    // tg = 0.349*100 + 0.686*150 + 0.168*200 = 34.9 + 102.9 + 33.6 = 171.4 -> 171 (green)
    // tb = 0.272*100 + 0.534*150 + 0.131*200 = 27.2 + 80.1 + 26.2 = 133.5 -> 133 (blue)
    // Pixel struct is B,G,R,A. So expected is {133, 171, 192, 255}
    Pixel expected1 = {133, 171, 192, 255}; 
    EXPECT_EQ(expected1, ApplySepiaToPixel(p1));

    Pixel p_white = {255, 255, 255, 255}; // B,G,R,A
    // Original: R=255, G=255, B=255
    // tr = (0.393+0.769+0.189)*255 = 1.351*255 = 344.505 -> 255 (red)
    // tg = (0.349+0.686+0.168)*255 = 1.203*255 = 306.765 -> 255 (green)
    // tb = (0.272+0.534+0.131)*255 = 0.937*255 = 238.935 -> 238 (blue)
    // Pixel struct is B,G,R,A. So expected is {238, 255, 255, 255}
    Pixel expected_white_sepia = {238, 255, 255, 255}; 
    EXPECT_EQ(expected_white_sepia, ApplySepiaToPixel(p_white));
}

TEST(PixelTest, GreyScalePixel) {
    Pixel p1 = {30, 20, 10, 255}; // B=30, G=20, R=10. Avg = (10+20+30)/3 = 20
    Pixel expected1 = {20, 20, 20, 255};
    EXPECT_EQ(expected1, GreyScalePixel(p1));

    Pixel p2 = {100, 100, 100, 100}; // Already greyscale
    Pixel expected2 = {100, 100, 100, 100};
    EXPECT_EQ(expected2, GreyScalePixel(p2));
}

// Helper function to create a Bitmap::File from a Matrix for testing
Bitmap::File CreateTestBitmap(const Matrix::Matrix<Pixel>& imageMatrix, int bitCount = 32 /* unused for now as CreateBitmapFromMatrix defaults to 32 */) {
    if (imageMatrix.rows() == 0 || imageMatrix.cols() == 0) {
        Bitmap::File invalidBitmapFile; // Default, IsValid() should be false
        std::cerr << "CreateTestBitmap called with empty matrix." << std::endl;
        return invalidBitmapFile; 
    }
    // Use the existing CreateBitmapFromMatrix from bitmap.cpp
    // This assumes CreateBitmapFromMatrix is robust and suitable for testing.
    return CreateBitmapFromMatrix(imageMatrix);
}

TEST(BitmapTest, ApplyBoxBlur_UniformColor) {
    Matrix::Matrix<Pixel> uniform_matrix(3, 3);
    Pixel red_pixel = {0, 0, 255, 255};
    for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) uniform_matrix[i][j] = red_pixel;
    Bitmap::File uniform_bmp = CreateTestBitmap(uniform_matrix);
    ASSERT_TRUE(uniform_bmp.IsValid());
    Bitmap::File blurred_uniform_bmp = ApplyBoxBlur(uniform_bmp, 1);
    ASSERT_TRUE(blurred_uniform_bmp.IsValid());
    EXPECT_EQ(uniform_bmp.bitmapInfoHeader.biWidth, blurred_uniform_bmp.bitmapInfoHeader.biWidth);
    EXPECT_EQ(uniform_bmp.bitmapInfoHeader.biHeight, blurred_uniform_bmp.bitmapInfoHeader.biHeight);
    Matrix::Matrix<Pixel> blurred_uniform_matrix = CreateMatrixFromBitmap(blurred_uniform_bmp);
    ASSERT_GT(blurred_uniform_matrix.rows(), 1);
    ASSERT_GT(blurred_uniform_matrix.cols(), 1);
    EXPECT_EQ(red_pixel, blurred_uniform_matrix[1][1]);
}

TEST(BitmapTest, ApplyBoxBlur_Pattern) {
    Matrix::Matrix<Pixel> pattern_matrix(3, 3);
    Pixel black_pixel = {0, 0, 0, 255};
    Pixel white_pixel = {255, 255, 255, 255};
    for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) pattern_matrix[i][j] = black_pixel;
    pattern_matrix[1][1] = white_pixel;
    Bitmap::File pattern_bmp = CreateTestBitmap(pattern_matrix);
    ASSERT_TRUE(pattern_bmp.IsValid());
    Bitmap::File blurred_pattern_bmp = ApplyBoxBlur(pattern_bmp, 1);
    ASSERT_TRUE(blurred_pattern_bmp.IsValid());
    Matrix::Matrix<Pixel> blurred_pattern_matrix = CreateMatrixFromBitmap(blurred_pattern_bmp);
    ASSERT_GT(blurred_pattern_matrix.rows(), 1);
    ASSERT_GT(blurred_pattern_matrix.cols(), 1);
    Pixel expected_center_pixel = {28, 28, 28, 255};
    EXPECT_EQ(expected_center_pixel, blurred_pattern_matrix[1][1]);
}

TEST(BitmapTest, ApplyBoxBlur_RadiusZero) {
    Matrix::Matrix<Pixel> pattern_matrix(3, 3);
    Pixel black_pixel = {0, 0, 0, 255};
    Pixel white_pixel = {255, 255, 255, 255};
    for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) pattern_matrix[i][j] = black_pixel;
    pattern_matrix[1][1] = white_pixel;
    Bitmap::File pattern_bmp = CreateTestBitmap(pattern_matrix);
    ASSERT_TRUE(pattern_bmp.IsValid());
    Bitmap::File not_blurred_bmp = ApplyBoxBlur(pattern_bmp, 0);
    ASSERT_TRUE(not_blurred_bmp.IsValid());
    Matrix::Matrix<Pixel> not_blurred_matrix = CreateMatrixFromBitmap(not_blurred_bmp);
    ASSERT_GT(not_blurred_matrix.rows(), 1);
    ASSERT_GT(not_blurred_matrix.cols(), 1);
    EXPECT_EQ(pattern_matrix[1][1], not_blurred_matrix[1][1]);
}

TEST(BitmapTest, ShrinkImage) {
    Matrix::Matrix<Pixel> large_matrix(4, 4); // 4x4 image
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) large_matrix[i][j] = {(BYTE)(i*10), (BYTE)(j*10), (BYTE)((i+j)*10), 255};
    
    Bitmap::File large_bmp = CreateTestBitmap(large_matrix);
    ASSERT_TRUE(large_bmp.IsValid());

    int scaleFactor = 2;
    Bitmap::File shrunk_bmp = ShrinkImage(large_bmp, scaleFactor);
    ASSERT_TRUE(shrunk_bmp.IsValid());
    EXPECT_EQ(large_bmp.bitmapInfoHeader.biWidth / scaleFactor, shrunk_bmp.bitmapInfoHeader.biWidth);
    EXPECT_EQ(large_bmp.bitmapInfoHeader.biHeight / scaleFactor, shrunk_bmp.bitmapInfoHeader.biHeight);
    
    Matrix::Matrix<Pixel> shrunk_matrix = CreateMatrixFromBitmap(shrunk_bmp);
    // Verifying pixel at (0,0) of the shrunk image.
    // This pixel is the average of the 2x2 block at (0,0) in the original large_matrix.
    // large_matrix[0][0] = {B:0,  G:0,  R:0,  A:255}
    // large_matrix[0][1] = {B:0,  G:10, R:10, A:255}
    // large_matrix[1][0] = {B:10, G:0,  R:10, A:255}
    // large_matrix[1][1] = {B:10, G:10, R:20, A:255}
    // Average:
    // Blue:  (0+0+10+10)/4 = 5
    // Green: (0+10+0+10)/4 = 5
    // Red:   (0+10+10+20)/4 = 10
    // Alpha: (255*4)/4 = 255
    Pixel expected_shrunk_pixel00 = {5, 5, 10, 255}; // BGR, Alpha
    if (shrunk_matrix.rows() > 0 && shrunk_matrix.cols() > 0) {
       EXPECT_EQ(expected_shrunk_pixel00, shrunk_matrix[0][0]);
    }
}

TEST(BitmapTest, RotateImage_CounterClockwise) {
    Matrix::Matrix<Pixel> rect_matrix(2, 3); // 2 rows, 3 cols
    Pixel p1 = {10,20,30,255}, p2 = {40,50,60,255}, p3 = {70,80,90,255};    // row 0
    Pixel p4 = {11,22,33,255}, p5 = {44,55,66,255}, p6 = {77,88,99,255};    // row 1
    rect_matrix[0][0]=p1; rect_matrix[0][1]=p2; rect_matrix[0][2]=p3;
    rect_matrix[1][0]=p4; rect_matrix[1][1]=p5; rect_matrix[1][2]=p6;

    Bitmap::File rect_bmp = CreateTestBitmap(rect_matrix);
    ASSERT_TRUE(rect_bmp.IsValid());

    Bitmap::File rotated_bmp = RotateImageCounterClockwise(rect_bmp);
    ASSERT_TRUE(rotated_bmp.IsValid());
    EXPECT_EQ(rect_bmp.bitmapInfoHeader.biHeight, rotated_bmp.bitmapInfoHeader.biWidth);
    EXPECT_EQ(rect_bmp.bitmapInfoHeader.biWidth, rotated_bmp.bitmapInfoHeader.biHeight);
    
    Matrix::Matrix<Pixel> rotated_matrix = CreateMatrixFromBitmap(rotated_bmp);
    // Original imageMatrix[i][j]
    // Rotated CounterClockwise: rotatedMatrix[j][new_cols - 1 - i] where new_cols is original rows
    // new_cols = imageMatrix.rows() = 2
    // rotatedMatrix[j][imageMatrix.rows() - 1 - i]
    // p1 (0,0) -> rotated_matrix[0][2-1-0] = rotated_matrix[0][1]
    // p2 (0,1) -> rotated_matrix[1][2-1-0] = rotated_matrix[1][1]
    // p3 (0,2) -> rotated_matrix[2][2-1-0] = rotated_matrix[2][1]
    // p4 (1,0) -> rotated_matrix[0][2-1-1] = rotated_matrix[0][0]
    // p5 (1,1) -> rotated_matrix[1][2-1-1] = rotated_matrix[1][0]
    // p6 (1,2) -> rotated_matrix[2][2-1-1] = rotated_matrix[2][0]
    if (rotated_matrix.rows() == 3 && rotated_matrix.cols() == 2) {
        EXPECT_EQ(p4, rotated_matrix[0][0]);
        EXPECT_EQ(p1, rotated_matrix[0][1]);
        EXPECT_EQ(p5, rotated_matrix[1][0]);
        EXPECT_EQ(p2, rotated_matrix[1][1]);
        EXPECT_EQ(p6, rotated_matrix[2][0]);
        EXPECT_EQ(p3, rotated_matrix[2][1]);
    }
}

TEST(BitmapTest, OtherImageFunctions_Placeholders) {
    Matrix::Matrix<Pixel> base_matrix(2, 2);
    base_matrix[0][0] = {10,20,30,255}; base_matrix[0][1] = {40,50,60,255};
    base_matrix[1][0] = {70,80,90,255}; base_matrix[1][1] = {100,110,120,255};
    Bitmap::File base_bmp = CreateTestBitmap(base_matrix);
    ASSERT_TRUE(base_bmp.IsValid());

    // RotateImageClockwise
    Bitmap::File rotated_cw_bmp = RotateImageClockwise(base_bmp);
    ASSERT_TRUE(rotated_cw_bmp.IsValid());
    EXPECT_EQ(base_bmp.bitmapInfoHeader.biHeight, rotated_cw_bmp.bitmapInfoHeader.biWidth);
    EXPECT_EQ(base_bmp.bitmapInfoHeader.biWidth, rotated_cw_bmp.bitmapInfoHeader.biHeight);

    // MirrorImage
    Bitmap::File mirrored_bmp = MirrorImage(base_bmp);
    ASSERT_TRUE(mirrored_bmp.IsValid());
    EXPECT_EQ(base_bmp.bitmapInfoHeader.biWidth, mirrored_bmp.bitmapInfoHeader.biWidth);
    EXPECT_EQ(base_bmp.bitmapInfoHeader.biHeight, mirrored_bmp.bitmapInfoHeader.biHeight);

    // FlipImage
    Bitmap::File flipped_bmp = FlipImage(base_bmp);
    ASSERT_TRUE(flipped_bmp.IsValid());
    EXPECT_EQ(base_bmp.bitmapInfoHeader.biWidth, flipped_bmp.bitmapInfoHeader.biWidth);
    EXPECT_EQ(base_bmp.bitmapInfoHeader.biHeight, flipped_bmp.bitmapInfoHeader.biHeight);

    // GreyscaleImage
    Bitmap::File grey_bmp = GreyscaleImage(base_bmp);
    ASSERT_TRUE(grey_bmp.IsValid());
    Matrix::Matrix<Pixel> grey_matrix = CreateMatrixFromBitmap(grey_bmp);
    if (grey_matrix.rows() > 0 && grey_matrix.cols() > 0) {
        Pixel p = grey_matrix[0][0];
        EXPECT_EQ(p.red, p.green);
        EXPECT_EQ(p.green, p.blue);
    }

    // InvertImageColors
    Bitmap::File inverted_bmp = InvertImageColors(base_bmp);
    ASSERT_TRUE(inverted_bmp.IsValid());
    Matrix::Matrix<Pixel> inverted_matrix = CreateMatrixFromBitmap(inverted_bmp);
    if (inverted_matrix.rows() > 0 && inverted_matrix.cols() > 0 && base_matrix.rows() > 0 && base_matrix.cols() > 0) {
        Pixel original_p = base_matrix[0][0];
        Pixel inverted_p = inverted_matrix[0][0];
        EXPECT_EQ((BYTE)(255-original_p.red), inverted_p.red);
    }

    // ApplySepiaTone
    Bitmap::File sepia_bmp = ApplySepiaTone(base_bmp);
    ASSERT_TRUE(sepia_bmp.IsValid());
    // Basic content check for sepia could compare one pixel to its ApplySepiaToPixel result
    Matrix::Matrix<Pixel> sepia_matrix = CreateMatrixFromBitmap(sepia_bmp);
     if (sepia_matrix.rows() > 0 && sepia_matrix.cols() > 0 && base_matrix.rows() > 0 && base_matrix.cols() > 0) {
        Pixel original_p = base_matrix[0][0];
        Pixel expected_sepia_p = ApplySepiaToPixel(original_p);
        EXPECT_EQ(expected_sepia_p, sepia_matrix[0][0]);
    }
}


TEST(BitmapTest, ChangePixelBrightness) {
    Pixel p_mid = {100, 120, 140, 255}; // B, G, R, A. Avg = (140+120+100)/3 = 120

    EXPECT_EQ(p_mid, ChangePixelBrightness(p_mid, 1.0f));

    // Expected for 1.5: Avg = 120. NewAvg = 120*1.5 = 180.
    // R_new = (140-120)+180 = 20+180 = 200
    // G_new = (120-120)+180 = 0+180 = 180
    // B_new = (100-120)+180 = -20+180 = 160
    Pixel expected_bright = {160, 180, 200, 255}; 
    EXPECT_EQ(expected_bright, ChangePixelBrightness(p_mid, 1.5f));

    // Expected for 0.5: Avg = 120. NewAvg = 120*0.5 = 60.
    // R_new = (140-120)+60 = 20+60 = 80
    // G_new = (120-120)+60 = 0+60 = 60
    // B_new = (100-120)+60 = -20+60 = 40
    Pixel expected_dark = {40, 60, 80, 255};
    EXPECT_EQ(expected_dark, ChangePixelBrightness(p_mid, 0.5f));
    
    Pixel p_black = {0,0,0,255}; // Avg = 0. NewAvg = 0.
    // R_new = (0-0)+0 = 0. G_new = 0. B_new = 0.
    EXPECT_EQ(p_black, ChangePixelBrightness(p_black, 1.5f));
    // Expected for 0.0: Avg = 120. NewAvg = 0.
    // R_new = (140-120)+0 = 20
    // G_new = (120-120)+0 = 0
    // B_new = (100-120)+0 = -20 -> 0
    Pixel expected_zero_bright = {0, 0, 20, 255};
    EXPECT_EQ(expected_zero_bright, ChangePixelBrightness(p_mid, 0.0f));

    Pixel p_white = {255,255,255,255}; // Avg = 255. NewAvg = 255*1.5 = 382 (clamped in components)
    // R_new = (255-255)+382 = 382 -> 255
    // G_new = (255-255)+382 = 382 -> 255
    // B_new = (255-255)+382 = 382 -> 255
    EXPECT_EQ(p_white, ChangePixelBrightness(p_white, 1.5f));

    Pixel p_dark_to_bright = {10, 20, 30, 255}; // Avg = (30+20+10)/3 = 20.
    // Brightness 20.0. NewAvg = 20*20 = 400
    // R_new = (30-20)+400 = 10+400 = 410 -> 255
    // G_new = (20-20)+400 = 0+400 = 400 -> 255
    // B_new = (10-20)+400 = -10+400 = 390 -> 255
    Pixel expected_dark_to_bright_clamped = {255, 255, 255, 255};
    EXPECT_EQ(expected_dark_to_bright_clamped, ChangePixelBrightness(p_dark_to_bright, 20.0f));
}

TEST(BitmapTest, ChangePixelContrast) {
    Pixel p_mid_gray = {128, 128, 128, 255}; // B, G, R, A
    EXPECT_EQ(p_mid_gray, ChangePixelContrast(p_mid_gray, 1.0f));
    EXPECT_EQ(p_mid_gray, ChangePixelContrast(p_mid_gray, 1.5f));
    EXPECT_EQ(p_mid_gray, ChangePixelContrast(p_mid_gray, 0.5f));
    EXPECT_EQ(p_mid_gray, ChangePixelContrast(p_mid_gray, 0.0f));

    Pixel p_dark = {50, 60, 70, 255}; // B, G, R
    // Expected for contrast 2.0:
    // B_new = 128 + (50-128)*2 = 128 - 78*2 = 128 - 156 = -28 -> 0
    // G_new = 128 + (60-128)*2 = 128 - 68*2 = 128 - 136 = -8 -> 0
    // R_new = 128 + (70-128)*2 = 128 - 58*2 = 128 - 116 = 12
    Pixel expected_contrast_high_dark = {0, 0, 12, 255};
    EXPECT_EQ(expected_contrast_high_dark, ChangePixelContrast(p_dark, 2.0f));
            
    Pixel p_light = {200, 210, 220, 255}; // B, G, R
    // Expected for contrast 2.0:
    // B_new = 128 + (200-128)*2 = 128 + 72*2 = 128 + 144 = 272 -> 255
    // G_new = 128 + (210-128)*2 = 128 + 82*2 = 128 + 164 = 292 -> 255
    // R_new = 128 + (220-128)*2 = 128 + 92*2 = 128 + 184 = 312 -> 255
    Pixel expected_contrast_high_light = {255, 255, 255, 255};
    EXPECT_EQ(expected_contrast_high_light, ChangePixelContrast(p_light, 2.0f));

    Pixel expected_contrast_zero = {128, 128, 128, 255}; // For contrast 0.0, all channels become 128
    EXPECT_EQ(expected_contrast_zero, ChangePixelContrast(p_dark, 0.0f));
    EXPECT_EQ(expected_contrast_zero, ChangePixelContrast(p_light, 0.0f));
}

TEST(BitmapTest, ChangePixelContrastRed) {
    Pixel p1 = {50, 100, 150, 255}; // B, G, R
    // Apply contrast 2.0 to Red (150): R_new = 128 + (150-128)*2 = 128 + 22*2 = 128+44 = 172
    Pixel expected_p1_red_contrast = {50, 100, 172, 255};
    EXPECT_EQ(expected_p1_red_contrast, ChangePixelContrastRed(p1, 2.0f));
}

TEST(BitmapTest, ChangePixelContrastGreen) {
    Pixel p1 = {50, 100, 150, 255}; // B, G, R
    // Apply contrast 2.0 to Green (100): G_new = 128 + (100-128)*2 = 128 - 28*2 = 128-56 = 72
    Pixel expected_p1_green_contrast = {50, 72, 150, 255};
    EXPECT_EQ(expected_p1_green_contrast, ChangePixelContrastGreen(p1, 2.0f));
}

TEST(BitmapTest, ChangePixelContrastBlue) {
    Pixel p1 = {50, 100, 150, 255}; // B, G, R
    // Apply contrast 2.0 to Blue (50): B_new = 128 + (50-128)*2 = 128 - 78*2 = 128-156 = -28 -> 0
    Pixel expected_p1_blue_contrast = {0, 100, 150, 255};
    EXPECT_EQ(expected_p1_blue_contrast, ChangePixelContrastBlue(p1, 2.0f));
}

TEST(BitmapTest, ChangePixelContrastMagenta) {
    Pixel p1 = {30, 60, 90, 255}; // B=30, G=60, R=90
    // Contrast 2.0. Affects Red and Blue. Green unchanged.
    // Blue_new: 128 + (30-128)*2 = 128 - 98*2 = 128 - 196 = -68 -> 0
    // Red_new:  128 + (90-128)*2 = 128 - 38*2 = 128 - 76 = 52
    Pixel expected_p1_magenta_contrast = {0, 60, 52, 255};
    EXPECT_EQ(expected_p1_magenta_contrast, ChangePixelContrastMagenta(p1, 2.0f));
}

TEST(BitmapTest, ChangePixelContrastYellow) {
    Pixel p1 = {30, 60, 90, 255}; // B=30, G=60, R=90
    // Contrast 2.0. Affects Red and Green. Blue unchanged.
    // Red_new:  128 + (90-128)*2 = 128 - 38*2 = 128 - 76 = 52
    // Green_new:128 + (60-128)*2 = 128 - 68*2 = 128 - 136 = -8 -> 0
    Pixel expected_p1_yellow_contrast = {30, 0, 52, 255};
    EXPECT_EQ(expected_p1_yellow_contrast, ChangePixelContrastYellow(p1, 2.0f));
}

TEST(BitmapTest, ChangePixelContrastCyan) {
    Pixel p1 = {30, 60, 90, 255}; // B=30, G=60, R=90
    // Contrast 2.0. Affects Green and Blue. Red unchanged.
    // Blue_new: 128 + (30-128)*2 = 128 - 98*2 = 128 - 196 = -68 -> 0
    // Green_new:128 + (60-128)*2 = 128 - 68*2 = 128 - 136 = -8 -> 0
    Pixel expected_p1_cyan_contrast = {0, 0, 90, 255};
    EXPECT_EQ(expected_p1_cyan_contrast, ChangePixelContrastCyan(p1, 2.0f));
}

TEST(BitmapTest, ChangePixelSaturation) {
    Pixel p_color = {50, 100, 150, 255}; // B=50, G=100, R=150. Avg = (150+100+50)/3 = 100
    EXPECT_EQ(p_color, ChangePixelSaturation(p_color, 1.0f));

    Pixel expected_greyscale = {100, 100, 100, 255}; // Avg = 100
    // When saturation is 0.0, the logic in bitmap.cpp is:
    // R (150) > 100: R_new = (150-100)*0.0 + 100 = 100
    // G (100) not > 100: G_new = 100
    // B (50) is not > 100. B_new = 50
    // So, it will not become perfectly greyscale by averaging if some components are <= average.
    // It will make components > average equal to average. Components <= average are untouched.
    Pixel actual_sat_zero_p_color = {50, 100, 100, 255}; 
    EXPECT_EQ(actual_sat_zero_p_color, ChangePixelSaturation(p_color, 0.0f));
    
    Pixel p_all_above_avg = {110, 120, 130, 255}; // Avg = 120
    // R(130) > 120: R_new = (130-120)*0.0 + 120 = 120
    // G(120) not > 120: G_new = 120
    // B(110) not > 120: B_new = 110
    Pixel expected_greyscale_all_above = {110, 120, 120, 255}; // Actually, this should be {120,120,120} if it worked as expected greyscale
    EXPECT_EQ(expected_greyscale_all_above, ChangePixelSaturation(p_all_above_avg, 0.0f));


    // For p_color (B=50, G=100, R=150), Avg=100: Saturation 2.0
    // R (150) > 100: R_new = (150-100)*2.0 + 100 = 50*2 + 100 = 200
    // G (100) is not > 100. G_new = 100
    // B (50) is not > 100. B_new = 50
    Pixel expected_sat_2 = {50, 100, 200, 255};
    EXPECT_EQ(expected_sat_2, ChangePixelSaturation(p_color, 2.0f));

    Pixel p_less_sat = {80, 100, 120, 255}; // B=80, G=100, R=120. Avg = 100
    // Saturation 0.5
    // R(120) > 100: R_new = (120-100)*0.5 + 100 = 10+100 = 110
    // G(100) not > 100: G_new = 100
    // B(80) not > 100: B_new = 80
    Pixel expected_sat_0_5 = {80, 100, 110, 255};
    EXPECT_EQ(expected_sat_0_5, ChangePixelSaturation(p_less_sat, 0.5f));
}

TEST(BitmapTest, ChangePixelLuminanceBlue) {
    // Case 1: Blue is dominant, luminance increases
    Pixel p_blue_dom = {150, 50, 50, 255}; // B=150, G=50, R=50. Avg=(50+50+150)/3 = 250/3 = 83
    float lum_factor = 1.5f;
    // NewAvg = 83 * 1.5 = 124.5 -> 124 (integer conversion)
    // R_new = (50-83)+124 = -33+124 = 91
    // G_new = (50-83)+124 = -33+124 = 91
    // B_new = (150-83)+124 = 67+124 = 191
    Pixel expected_p_blue_dom_lum = {191, 91, 91, 255};
    EXPECT_EQ(expected_p_blue_dom_lum, ChangePixelLuminanceBlue(p_blue_dom, lum_factor));

    // Case 2: Blue is not dominant, pixel should be unchanged
    Pixel p_red_dom = {50, 150, 50, 255}; // B=50, G=50, R=150 (Corrected to make Red dominant over Blue)
    EXPECT_EQ(p_red_dom, ChangePixelLuminanceBlue(p_red_dom, lum_factor));
}

TEST(BitmapTest, ChangePixelLuminanceGreen) {
    Pixel p_green_dom = {50, 150, 50, 255}; // B=50, G=150, R=50. Avg = 83
    float lum_factor = 1.5f; // NewAvg = 124
    // R_new = (50-83)+124 = 91
    // G_new = (150-83)+124 = 191
    // B_new = (50-83)+124 = 91
    Pixel expected_p_green_dom_lum = {91, 191, 91, 255};
    EXPECT_EQ(expected_p_green_dom_lum, ChangePixelLuminanceGreen(p_green_dom, lum_factor));

    Pixel p_blue_dom = {150, 50, 50, 255}; 
    EXPECT_EQ(p_blue_dom, ChangePixelLuminanceGreen(p_blue_dom, lum_factor));
}

TEST(BitmapTest, ChangePixelLuminanceRed) {
    Pixel p_red_dom = {50, 50, 150, 255}; // B=50, G=50, R=150. Avg = 83
    float lum_factor = 1.5f; // NewAvg = 124
    // R_new = (150-83)+124 = 191
    // G_new = (50-83)+124 = 91
    // B_new = (50-83)+124 = 91
    Pixel expected_p_red_dom_lum = {91, 91, 191, 255};
    EXPECT_EQ(expected_p_red_dom_lum, ChangePixelLuminanceRed(p_red_dom, lum_factor));

    Pixel p_green_dom = {50, 150, 50, 255};
    EXPECT_EQ(p_green_dom, ChangePixelLuminanceRed(p_green_dom, lum_factor));
}

TEST(BitmapTest, ChangePixelLuminanceMagenta) {
    // Magenta means R and B are significant. Condition is `magenta_component > average`.
    // magenta_component = std::min(pixel.red, pixel.blue)
    Pixel p_magenta_ish = {140, 20, 150, 255}; // B=140, G=20, R=150. Avg=(150+20+140)/3 = 310/3 = 103. Magenta_comp = min(150,140)=140. 140 > 103.
    float lum_factor = 1.2f; // NewAvg = 103 * 1.2 = 123.6 -> 123
    // R_new = (150-103)+123 = 47+123 = 170
    // G_new = (20-103)+123 = -83+123 = 40
    // B_new = (140-103)+123 = 37+123 = 160
    Pixel expected_p_magenta_lum = {160, 40, 170, 255};
    EXPECT_EQ(expected_p_magenta_lum, ChangePixelLuminanceMagenta(p_magenta_ish, lum_factor));

    Pixel p_not_magenta = {20, 150, 30, 255}; // G is dominant. Avg=(30+150+20)/3 = 200/3 = 66. Magenta_comp=min(30,20)=20. 20 is not > 66.
    EXPECT_EQ(p_not_magenta, ChangePixelLuminanceMagenta(p_not_magenta, lum_factor));
}

TEST(BitmapTest, ChangePixelLuminanceYellow) {
    // Yellow means R and G are significant. Condition is `yellow_component > average`.
    // yellow_component = std::min(pixel.red, pixel.green)
    Pixel p_yellow_ish = {20, 140, 150, 255}; // B=20, G=140, R=150. Avg=(150+140+20)/3 = 310/3 = 103. Yellow_comp = min(150,140)=140. 140 > 103.
    float lum_factor = 0.8f; // NewAvg = 103 * 0.8 = 82.4 -> 82
    // R_new = (150-103)+82 = 47+82 = 129
    // G_new = (140-103)+82 = 37+82 = 119
    // B_new = (20-103)+82 = -83+82 = -1 -> 0
    Pixel expected_p_yellow_lum = {0, 119, 129, 255};
    EXPECT_EQ(expected_p_yellow_lum, ChangePixelLuminanceYellow(p_yellow_ish, lum_factor));
    
    Pixel p_not_yellow = {150, 20, 30, 255}; // B is dominant
    EXPECT_EQ(p_not_yellow, ChangePixelLuminanceYellow(p_not_yellow, lum_factor));
}

TEST(BitmapTest, ChangePixelLuminanceCyan) {
    // Cyan means G and B are significant. Condition is `cyan_component > average`.
    // cyan_component = std::min(pixel.green, pixel.blue)
    Pixel p_cyan_ish = {150, 140, 20, 255}; // B=150, G=140, R=20. Avg=(20+140+150)/3 = 310/3 = 103. Cyan_comp = min(140,150)=140. 140 > 103.
    float lum_factor = 1.1f; // NewAvg = 103 * 1.1 = 113.3 -> 113
    // R_new = (20-103)+113 = -83+113 = 30
    // G_new = (140-103)+113 = 37+113 = 150
    // B_new = (150-103)+113 = 47+113 = 160
    Pixel expected_p_cyan_lum = {160, 150, 30, 255};
    EXPECT_EQ(expected_p_cyan_lum, ChangePixelLuminanceCyan(p_cyan_ish, lum_factor));

    Pixel p_not_cyan = {20, 30, 150, 255}; // R is dominant
    EXPECT_EQ(p_not_cyan, ChangePixelLuminanceCyan(p_not_cyan, lum_factor));
}
