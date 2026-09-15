#include "bitmap_file.h"
#include <fstream>
#include <vector>
#include <cmath>   // For std::abs
#include <iostream> // For debugging, remove later
#include "../safe_math.hpp"

Bitmap::File::File() : isValid(false)
{
}

Bitmap::File::File(std::string filename) : isValid(false)
{
    Open(filename);
}

Bitmap::File::~File()
{
}

bool Bitmap::File::Save()
{
    if (!isValid) {
        return false;
    }
    if (bitmapFilename.empty()) {
        return false; 
    }
    uint32_t abs_height = 0;
    if (!BmpTool::SafeMath::getSafeAbsoluteHeight(bitmapInfoHeader.biHeight, abs_height)) {
        return false;
    }
    if (bitmapInfoHeader.biWidth <= 0 || static_cast<uint32_t>(bitmapInfoHeader.biWidth) > BmpTool::SafeMath::MAX_SAFE_DIMENSION) {
        return false; 
    }
    if (bitmapInfoHeader.biBitCount != 24 && bitmapInfoHeader.biBitCount != 32) {
        return false; 
    }

    std::ofstream file(bitmapFilename, std::ios::binary);
    if (!file) {
        return false;
    }

    uint32_t bytesPerPixel = bitmapInfoHeader.biBitCount / 8;
    uint32_t rowBytesPadded = 0;
    if (!BmpTool::SafeMath::computeRowStride(static_cast<uint32_t>(bitmapInfoHeader.biWidth), bitmapInfoHeader.biBitCount, rowBytesPadded)) {
        return false;
    }
    uint32_t rowBytesUnpadded = static_cast<uint32_t>(bitmapInfoHeader.biWidth) * bytesPerPixel;
    uint32_t paddingPerRow = rowBytesPadded - rowBytesUnpadded;

    // Prepare headers for writing
    BITMAPFILEHEADER writeFileHeader = bitmapFileHeader;
    BITMAPINFOHEADER writeInfoHeader = bitmapInfoHeader;

    uint32_t biSizeImage = 0;
    if (!BmpTool::SafeMath::multiply(rowBytesPadded, abs_height, biSizeImage) || biSizeImage > BmpTool::SafeMath::MAX_SAFE_IMAGE_BYTES) {
        return false;
    }
    writeInfoHeader.biSizeImage = biSizeImage;
    writeFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    size_t bfSize = 0;
    if (!BmpTool::SafeMath::add(static_cast<size_t>(writeFileHeader.bfOffBits), static_cast<size_t>(biSizeImage), bfSize) || bfSize > std::numeric_limits<uint32_t>::max()) {
        return false;
    }
    writeFileHeader.bfSize = static_cast<uint32_t>(bfSize);
    writeFileHeader.bfType = 0x4D42; // 'BM'
    writeFileHeader.bfReserved1 = 0;
    writeFileHeader.bfReserved2 = 0;

    file.write(reinterpret_cast<const char*>(&writeFileHeader), sizeof(BITMAPFILEHEADER));
    if (!file) { return false; }

    file.write(reinterpret_cast<const char*>(&writeInfoHeader), sizeof(BITMAPINFOHEADER));
    if (!file) { return false; }

    const unsigned char* current_pixel_data = bitmapData.data();
    size_t required_data_size = 0;
    if (!BmpTool::SafeMath::multiply(static_cast<size_t>(abs_height), static_cast<size_t>(rowBytesUnpadded), required_data_size)) {
        return false;
    }
    
    if (bitmapData.size() < required_data_size) {
        return false;
    }

    unsigned char padding_bytes[3] = {0,0,0}; 

    for (uint32_t i = 0; i < abs_height; ++i) {
        file.write(reinterpret_cast<const char*>(current_pixel_data), rowBytesUnpadded);
        if (!file) { return false; }
        
        if (paddingPerRow > 0) {
            file.write(reinterpret_cast<const char*>(padding_bytes), paddingPerRow); 
            if (!file) { return false; }
        }
        current_pixel_data += rowBytesUnpadded;
    }

    file.close();
    return true;
}

bool Bitmap::File::SaveAs(std::string filename)
{
    Bitmap::File::bitmapFilename = filename;
    return Bitmap::File::Save(); 
}

bool Bitmap::File::Open(std::string filename)
{
    isValid = false; 
    Bitmap::File::bitmapFilename = filename;
    std::ifstream file(bitmapFilename, std::ios::binary);
    if (!file) {
        return false;
    }

    file.read(reinterpret_cast<char*>(&bitmapFileHeader), sizeof(BITMAPFILEHEADER));
    if (!file || file.gcount() != sizeof(BITMAPFILEHEADER)) {
        file.close(); return false;
    }

    if (bitmapFileHeader.bfType != 0x4D42) { 
        file.close(); return false;
    }

    file.read(reinterpret_cast<char*>(&bitmapInfoHeader), sizeof(BITMAPINFOHEADER));
    if (!file || file.gcount() != sizeof(BITMAPINFOHEADER)) {
        file.close(); return false;
    }

    if (bitmapInfoHeader.biSize < sizeof(BITMAPINFOHEADER)) {
        file.close(); return false;
    }
    if (bitmapInfoHeader.biCompression != 0) { // Strictly uncompressed BI_RGB only
        file.close(); return false;
    }
    if (bitmapInfoHeader.biPlanes != 1) {
        file.close(); return false;
    }

    uint32_t abs_height = 0;
    if (!BmpTool::SafeMath::getSafeAbsoluteHeight(bitmapInfoHeader.biHeight, abs_height)) {
        file.close(); return false;
    }
    if (bitmapInfoHeader.biWidth <= 0 || static_cast<uint32_t>(bitmapInfoHeader.biWidth) > BmpTool::SafeMath::MAX_SAFE_DIMENSION) {
        file.close(); return false;
    }
    if (bitmapInfoHeader.biBitCount != 24 && bitmapInfoHeader.biBitCount != 32) { 
        file.close(); return false;
    }
    
    uint32_t bytesPerPixel = bitmapInfoHeader.biBitCount / 8;
    uint32_t rowBytesPadded = 0;
    if (!BmpTool::SafeMath::computeRowStride(static_cast<uint32_t>(bitmapInfoHeader.biWidth), bitmapInfoHeader.biBitCount, rowBytesPadded)) {
        file.close(); return false;
    }
    uint32_t rowBytesUnpadded = static_cast<uint32_t>(bitmapInfoHeader.biWidth) * bytesPerPixel;
    uint32_t paddingPerRow = rowBytesPadded - rowBytesUnpadded; 

    uint32_t expectedBiSizeImage = 0;
    if (!BmpTool::SafeMath::multiply(rowBytesPadded, abs_height, expectedBiSizeImage) || expectedBiSizeImage > BmpTool::SafeMath::MAX_SAFE_IMAGE_BYTES) {
        file.close(); return false;
    }
    if (bitmapInfoHeader.biCompression == 0) { 
        if (bitmapInfoHeader.biSizeImage == 0) {
            bitmapInfoHeader.biSizeImage = expectedBiSizeImage;
        }
    }

    uint32_t actualPixelDataSize = 0;
    if (!BmpTool::SafeMath::multiply(rowBytesUnpadded, abs_height, actualPixelDataSize) || actualPixelDataSize > BmpTool::SafeMath::MAX_SAFE_IMAGE_BYTES) {
        file.close(); return false;
    }
    bitmapData.resize(actualPixelDataSize);
    
    if (actualPixelDataSize == 0) { 
        isValid = true; 
        file.close();
        return true;
    }

    file.seekg(bitmapFileHeader.bfOffBits, std::ios::beg);
    if (!file) {
        bitmapData.clear(); file.close(); return false;
    }

    unsigned char* currentRowDest = bitmapData.data();
    unsigned char padding_buffer[3] = {0,0,0}; 

    for (uint32_t i = 0; i < abs_height; ++i) {
        file.read(reinterpret_cast<char*>(currentRowDest), rowBytesUnpadded);
        if (!file || file.gcount() != static_cast<std::streamsize>(rowBytesUnpadded)) {
            bitmapData.clear(); file.close(); return false;
        }
        
        if (paddingPerRow > 0) { 
             file.read(reinterpret_cast<char*>(padding_buffer), paddingPerRow);
             if (file.gcount() != static_cast<std::streamsize>(paddingPerRow) && !file.eof()) { 
                 bitmapData.clear(); file.close(); return false;
             }
             if (file.gcount() < static_cast<std::streamsize>(paddingPerRow) && file.eof() && i < (abs_height - 1)) {
                 bitmapData.clear(); file.close(); return false;
             }
        }
        currentRowDest += rowBytesUnpadded;
    }

    file.close();
    isValid = true;
    return true;
}

void Bitmap::File::Rename(std::string filename)
{
    Bitmap::File::bitmapFilename = filename;
}

std::string Bitmap::File::Filename() // Removed const
{
    return Bitmap::File::bitmapFilename;
}

bool Bitmap::File::IsValid() // Removed const
{
    return Bitmap::File::isValid;
}

void Bitmap::File::SetValid()
{
    Bitmap::File::isValid = true;
}