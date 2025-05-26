#pragma once
#ifndef BITMAP_FILE
#define BITMAP_FILE

#include <cstdint> // Include for standard integer types
#include <vector>
#include <string>

// Define Windows-specific types using standard C++ types
typedef uint8_t  BYTE;
typedef uint32_t DWORD;
typedef int32_t  LONG;
typedef uint16_t WORD;

#pragma pack(push, 1) // Ensure structure is packed
typedef struct tagBITMAPFILEHEADER {
    WORD    bfType;        // Specifies the file type, must be BM.
    DWORD   bfSize;        // Specifies the size, in bytes, of the bitmap file.
    WORD    bfReserved1;   // Reserved; must be zero.
    WORD    bfReserved2;   // Reserved; must be zero.
    DWORD   bfOffBits;     // Specifies the offset, in bytes, from the beginning of the BITMAPFILEHEADER structure to the bitmap bits.
} BITMAPFILEHEADER;
#pragma pack(pop)

#pragma pack(push, 1) // Ensure structure is packed
typedef struct tagBITMAPINFOHEADER {
    DWORD      biSize;         // Specifies the number of bytes required by the structure.
    LONG       biWidth;        // Specifies the width of the bitmap, in pixels.
    LONG       biHeight;       // Specifies the height of the bitmap, in pixels.
    WORD       biPlanes;       // Specifies the number of planes for the target device. This value must be set to 1.
    WORD       biBitCount;     // Specifies the number of bits-per-pixel.
    DWORD      biCompression;  // Specifies the type of compression for a compressed bottom-up bitmap.
    DWORD      biSizeImage;    // Specifies the size, in bytes, of the image. This may be set to zero for BI_RGB bitmaps.
    LONG       biXPelsPerMeter;// Specifies the horizontal resolution, in pixels-per-meter, of the target device for the bitmap.
    LONG       biYPelsPerMeter;// Specifies the vertical resolution, in pixels-per-meter, of the target device for the bitmap.
    DWORD      biClrUsed;      // Specifies the number of color indexes in the color table that are actually used by the bitmap.
    DWORD      biClrImportant; // Specifies the number of color indexes that are considered important for displaying the bitmap.
} BITMAPINFOHEADER;
#pragma pack(pop)

namespace Bitmap
{
    class File
    {
    public:
        BITMAPINFOHEADER bitmapInfoHeader; // Replaced BITMAPINFO
        BITMAPFILEHEADER bitmapFileHeader;
        std::vector<uint8_t> bitmapData; // Changed BYTE to uint8_t

        File();
        File(std::string filename);
        ~File();
        bool Save();
        bool SaveAs(std::string filename);
        void Rename(std::string filename);
        bool Open(std::string filename);
        bool IsValid();
        void SetValid();
        std::string Filename();

    private:
        std::string bitmapFilename;
        bool isValid = false;
    };
}

#endif