#include "src/bitmap.h"

int main()
{
    Bitmap::File bitmapFile;
    bitmapFile.Open("test.bmp");
    bitmapFile = ShrinkImage(bitmapFile, 5);
   //bitmapFile = GreyscaleImage(bitmapFile);

    bitmapFile = ChangeImageLuminanceYellow(bitmapFile, 2);
    bitmapFile = ChangeImageSaturationYellow(bitmapFile, 0);
    bitmapFile.SaveAs("test2.bmp");

    return 0;
}