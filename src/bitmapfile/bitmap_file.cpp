#include "bitmap_file.h"
#include <fstream> // Required for std::ofstream and std::ifstream

Bitmap::File::File()
{
}

Bitmap::File::File(std::string filename)
{
    Bitmap::File::Open(filename);
}

Bitmap::File::~File()
{
}

bool Bitmap::File::Save()
{
    if (!Bitmap::File::isValid)
    {
        return false;
    }

    std::ofstream file(bitmapFilename, std::ios::binary);
    if (!file)
    {
        return false;
    }

    file.write(reinterpret_cast<char*>(&bitmapFileHeader), sizeof(BITMAPFILEHEADER));
    if (!file)
    {
        return false;
    }

    file.write(reinterpret_cast<char*>(&bitmapInfoHeader), sizeof(BITMAPINFOHEADER));
    if (!file)
    {
        return false;
    }

    file.write(reinterpret_cast<char*>(bitmapData.data()), bitmapData.size());
    if (!file)
    {
        return false;
    }

    file.close();
    return true;
}

bool Bitmap::File::SaveAs(std::string filename)
{
    Bitmap::File::bitmapFilename = filename;
    return Bitmap::File::Save(); // Now SaveAs can just call Save
}

bool Bitmap::File::Open(std::string filename)
{
    Bitmap::File::bitmapFilename = filename;
    std::ifstream file(bitmapFilename, std::ios::binary);
    if (!file)
    {
        isValid = false;
        return false;
    }

    file.read(reinterpret_cast<char*>(&bitmapFileHeader), sizeof(BITMAPFILEHEADER));
    if (!file)
    {
        isValid = false;
        return false;
    }

    // Check for 'BM' signature
    if (bitmapFileHeader.bfType != 0x4D42) { // 'BM' in hex
        isValid = false;
        return false;
    }

    file.read(reinterpret_cast<char*>(&bitmapInfoHeader), sizeof(BITMAPINFOHEADER));
    if (!file)
    {
        isValid = false;
        return false;
    }

    bitmapData.resize(bitmapFileHeader.bfSize - bitmapFileHeader.bfOffBits);
    file.read(reinterpret_cast<char*>(bitmapData.data()), bitmapData.size());
    if (!file)
    {
        isValid = false;
        return false;
    }

    file.close();
    isValid = true;
    return true;
}

void Bitmap::File::Rename(std::string filename)
{
    Bitmap::File::bitmapFilename = filename;
}

std::string Bitmap::File::Filename()
{
    return Bitmap::File::bitmapFilename;
}

bool Bitmap::File::IsValid()
{
    return Bitmap::File::isValid;
}

void Bitmap::File::SetValid()
{
    Bitmap::File::isValid = true;
}