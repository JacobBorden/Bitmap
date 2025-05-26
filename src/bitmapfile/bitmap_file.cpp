#include "bitmap_file.h"
#include <fstream>
#include <vector>
#include <cmath>   // For std::abs
#include <iostream> // For debugging, remove later

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
    // Basic validation for image properties that affect saving
    if (bitmapInfoHeader.biWidth <= 0 || bitmapInfoHeader.biHeight == 0 || 
        (bitmapInfoHeader.biBitCount != 24 && bitmapInfoHeader.biBitCount != 32)) {
        // std::cerr << "Save error: Invalid image dimensions or bit count for saving." << std::endl;
        return false; 
    }


    std::ofstream file(bitmapFilename, std::ios::binary);
    if (!file) {
        return false;
    }

    uint32_t bytesPerPixel = bitmapInfoHeader.biBitCount / 8;
    // This check is technically redundant due to the one at the start of the function, but kept for safety.
    if (bytesPerPixel == 0) { 
        return false;
    }

    uint32_t rowBytesUnpadded = bitmapInfoHeader.biWidth * bytesPerPixel;
    uint32_t rowBytesPadded = (rowBytesUnpadded + 3) & ~3; 
    uint32_t paddingPerRow = rowBytesPadded - rowBytesUnpadded;

    // Prepare headers for writing
    BITMAPFILEHEADER writeFileHeader = bitmapFileHeader;
    BITMAPINFOHEADER writeInfoHeader = bitmapInfoHeader;

    writeInfoHeader.biSizeImage = rowBytesPadded * std::abs(writeInfoHeader.biHeight);
    writeFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    writeFileHeader.bfSize = writeFileHeader.bfOffBits + writeInfoHeader.biSizeImage;
    writeFileHeader.bfType = 0x4D42; // 'BM'
    writeFileHeader.bfReserved1 = 0;
    writeFileHeader.bfReserved2 = 0;


    file.write(reinterpret_cast<const char*>(&writeFileHeader), sizeof(BITMAPFILEHEADER));
    if (!file) { return false; }

    file.write(reinterpret_cast<const char*>(&writeInfoHeader), sizeof(BITMAPINFOHEADER));
    if (!file) { return false; }

    const unsigned char* current_pixel_data = bitmapData.data();
    // Ensure bitmapData has enough data for what headers claim
    if (bitmapData.size() < std::abs(writeInfoHeader.biHeight) * rowBytesUnpadded) {
        // std::cerr << "Save error: bitmapData size is insufficient for declared dimensions." << std::endl;
        return false;
    }

    unsigned char padding_bytes[3] = {0,0,0}; 

    for (int i = 0; i < std::abs(writeInfoHeader.biHeight); ++i) {
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

    if (bitmapInfoHeader.biWidth <= 0 || bitmapInfoHeader.biHeight == 0 || 
        (bitmapInfoHeader.biBitCount != 24 && bitmapInfoHeader.biBitCount != 32)) { 
        file.close(); return false;
    }
    
    uint32_t bytesPerPixel = bitmapInfoHeader.biBitCount / 8;

    uint32_t rowBytesUnpadded = bitmapInfoHeader.biWidth * bytesPerPixel;
    uint32_t rowBytesPadded = (rowBytesUnpadded + 3) & ~3;
    uint32_t paddingPerRow = rowBytesPadded - rowBytesUnpadded; 

    uint32_t expectedBiSizeImage = rowBytesPadded * std::abs(bitmapInfoHeader.biHeight);
    if (bitmapInfoHeader.biCompression == 0 ) { 
        if (bitmapInfoHeader.biSizeImage == 0) {
            bitmapInfoHeader.biSizeImage = expectedBiSizeImage;
        } else if (bitmapInfoHeader.biSizeImage != expectedBiSizeImage) {
            // Discrepancy. This could be an issue.
            // For now, we'll trust calculated actualPixelDataSize for resizing bitmapData.
        }
    }
   
    // Optional: More robust check for bfSize against expected values
    // if (bitmapFileHeader.bfSize != bitmapFileHeader.bfOffBits + bitmapInfoHeader.biSizeImage) {
    //     // Potentially problematic.
    // }

    uint32_t actualPixelDataSize = std::abs(bitmapInfoHeader.biHeight) * rowBytesUnpadded;
    bitmapData.resize(actualPixelDataSize);
    
    if (actualPixelDataSize == 0) { 
        // This handles images with 0 width or 0 height correctly.
        // An "empty" image (0 pixels) can be considered valid.
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

    for (int i = 0; i < std::abs(bitmapInfoHeader.biHeight); ++i) {
        file.read(reinterpret_cast<char*>(currentRowDest), rowBytesUnpadded);
        if (!file || file.gcount() != rowBytesUnpadded) {
            bitmapData.clear(); file.close(); return false;
        }
        
        if (paddingPerRow > 0) { 
             file.read(reinterpret_cast<char*>(padding_buffer), paddingPerRow);
             // EOF is only okay if it's the very last read of the file.
             // gcount() != paddingPerRow could be true if EOF was hit mid-padding.
             if (file.gcount() != paddingPerRow && !file.eof()) { 
                 bitmapData.clear(); file.close(); return false;
             }
             // If gcount is less than paddingPerRow but it is EOF, it implies a truncated file.
             // This might be an error condition depending on strictness. For now, accept if pixels are read.
             if (file.gcount() < paddingPerRow && file.eof() && i < (std::abs(bitmapInfoHeader.biHeight) -1) ){
                 // If EOF hit during padding but it's not the last row, file is truncated.
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