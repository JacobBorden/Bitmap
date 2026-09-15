#include "bitmap.h"
#include <iostream>

int main()
{
    Bitmap::File bitmapFile;
    if (!bitmapFile.Open("test.bmp"))
    {
        std::cout << "test.bmp not found, creating synthetic 100x100 test image...\n";
        Matrix::Matrix<Pixel> sample(100, 100);
        for (size_t r = 0; r < sample.rows(); ++r)
        {
            for (size_t c = 0; c < sample.cols(); ++c)
            {
                sample[r][c] = Pixel{
                    static_cast<BYTE>(r * 255 / 100),
                    static_cast<BYTE>(c * 255 / 100),
                    static_cast<BYTE>((r + c) * 255 / 200),
                    255
                };
            }
        }
        bitmapFile = CreateBitmapFromMatrix(sample);
        bitmapFile.SaveAs("test.bmp");
    }

    bitmapFile = ShrinkImage(bitmapFile, 2);
    bitmapFile = ChangeImageLuminanceYellow(bitmapFile, 2.0f);
    bitmapFile = ChangeImageSaturationYellow(bitmapFile, 0.0f);
    bitmapFile.SaveAs("test2.bmp");
    std::cout << "Successfully processed image and saved test2.bmp\n";

    return 0;
}