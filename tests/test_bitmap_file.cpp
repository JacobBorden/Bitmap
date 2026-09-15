#include <fstream>
#include <cstdio> // For std::remove
#include "gtest/gtest.h"
#include "../src/bitmapfile/bitmap_file.h" // Adjust path as necessary

// Helper function to create a minimal 1x1 red 24-bit BMP file
// Returns true on success, false on failure
bool CreateMinimalValidBmp(const std::string& filepath, int width, int height, unsigned char r, unsigned char g, unsigned char b) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    BITMAPFILEHEADER bfh; // Removed Bitmap::
    BITMAPINFOHEADER bih; // Removed Bitmap::

    int imageRowSize = ((width * bih.biBitCount + 31) / 32) * 4; // Row size must be a multiple of 4 bytes
    int imageSize = imageRowSize * height;


    bfh.bfType = 0x4D42; // 'BM'
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + imageSize; // Removed Bitmap::
    bfh.bfReserved1 = 0;
    bfh.bfReserved2 = 0;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); // Removed Bitmap::

    bih.biSize = sizeof(BITMAPINFOHEADER); // Removed Bitmap::
    bih.biWidth = width;
    bih.biHeight = height;
    bih.biPlanes = 1;
    bih.biBitCount = 24; // 24 bits per pixel
    bih.biCompression = 0; // BI_RGB (no compression)
    bih.biSizeImage = imageSize; 
    bih.biXPelsPerMeter = 0; // Typically 0
    bih.biYPelsPerMeter = 0; // Typically 0
    bih.biClrUsed = 0;       // Not using a color palette
    bih.biClrImportant = 0;  // All colors are important

    file.write(reinterpret_cast<const char*>(&bfh), sizeof(bfh));
    file.write(reinterpret_cast<const char*>(&bih), sizeof(bih));

    // Pixel data (BGR order)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            file.write(reinterpret_cast<const char*>(&b), 1);
            file.write(reinterpret_cast<const char*>(&g), 1);
            file.write(reinterpret_cast<const char*>(&r), 1);
        }
        // Add padding if row size is not a multiple of 4
        for (int k = 0; k < (imageRowSize - (width * 3)); ++k) {
             char paddingByte = 0;
             file.write(&paddingByte, 1);
        }
    }

    file.close();
    return true;
}


// Test suite for Bitmap::File
class BitmapFileTest : public ::testing::Test {
protected:
    // You can define per-test-suite helper functions or member variables here
    const std::string temp_valid_bmp = "temp_valid_test.bmp";
    const std::string temp_save_as_bmp = "test_save_as.bmp";
    const std::string temp_save_bmp = "test_save.bmp";
    const std::string temp_rename_initial_bmp = "rename_initial.bmp";


    void TearDown() override {
        // Clean up temporary files created by tests
        std::remove(temp_valid_bmp.c_str());
        std::remove(temp_save_as_bmp.c_str());
        std::remove(temp_save_bmp.c_str());
        std::remove(temp_rename_initial_bmp.c_str());
        std::remove("rename_final.bmp"); // If rename test created it
    }
};

// Constructor Tests
TEST_F(BitmapFileTest, DefaultConstructor) {
    Bitmap::File bf;
    EXPECT_FALSE(bf.IsValid());
    EXPECT_TRUE(bf.Filename().empty());
}

TEST_F(BitmapFileTest, FilenameConstructor) {
    Bitmap::File bf("test.bmp");
    EXPECT_EQ("test.bmp", bf.Filename());
    EXPECT_FALSE(bf.IsValid()); // Constructor with filename doesn't open or validate
}

// Open Tests
TEST_F(BitmapFileTest, OpenNonExistentFile) {
    Bitmap::File bf;
    EXPECT_FALSE(bf.Open("non_existent_rubbish_temp.bmp"));
    EXPECT_FALSE(bf.IsValid());
}

TEST_F(BitmapFileTest, OpenValidBmp) {
    // Create a minimal valid BMP file
    ASSERT_TRUE(CreateMinimalValidBmp(temp_valid_bmp, 1, 1, 255, 0, 0)); // 1x1 red pixel

    Bitmap::File bf_valid;
    EXPECT_TRUE(bf_valid.Open(temp_valid_bmp.c_str()));
    EXPECT_TRUE(bf_valid.IsValid());
    EXPECT_EQ(temp_valid_bmp, bf_valid.Filename());

    // Check some header values
    EXPECT_EQ(bf_valid.bitmapFileHeader.bfType, 0x4D42);
    EXPECT_EQ(bf_valid.bitmapInfoHeader.biWidth, 1);
    EXPECT_EQ(bf_valid.bitmapInfoHeader.biHeight, 1);
    EXPECT_EQ(bf_valid.bitmapInfoHeader.biBitCount, 24);
    EXPECT_EQ(bf_valid.bitmapData.size(), 3); // 1 pixel * 3 bytes (BGR)
    if (bf_valid.bitmapData.size() == 3) {
        EXPECT_EQ(bf_valid.bitmapData[0], 0);   // Blue
        EXPECT_EQ(bf_valid.bitmapData[1], 0);   // Green
        EXPECT_EQ(bf_valid.bitmapData[2], 255); // Red
    }
}

// IsValid/SetValid Tests
TEST_F(BitmapFileTest, IsValidSetValid) {
    Bitmap::File bf;
    EXPECT_FALSE(bf.IsValid());
    bf.SetValid();
    EXPECT_TRUE(bf.IsValid());
    // bf.SetValid(false); // Removed: SetValid does not take an argument.
    // To make it invalid again for testing, typically re-initialize or use a failed operation.
    // For this test, simply checking SetValid() and IsValid() is sufficient.
}

// Save/SaveAs Tests
TEST_F(BitmapFileTest, SaveAsAndReload) {
    Bitmap::File bf_save;

    // Populate headers for a 1x1 red 24-bit BMP
    bf_save.bitmapFileHeader.bfType = 0x4D42;
    bf_save.bitmapFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); // Removed Bitmap::
    
    bf_save.bitmapInfoHeader.biSize = sizeof(BITMAPINFOHEADER); // Removed Bitmap::
    bf_save.bitmapInfoHeader.biWidth = 1;
    bf_save.bitmapInfoHeader.biHeight = 1;
    bf_save.bitmapInfoHeader.biPlanes = 1;
    bf_save.bitmapInfoHeader.biBitCount = 24;
    bf_save.bitmapInfoHeader.biCompression = 0; // BI_RGB
    
    // For a 1x1 24bpp image, row size is 3 bytes. Padded to 4 bytes.
    uint32_t rowSize = ((bf_save.bitmapInfoHeader.biWidth * bf_save.bitmapInfoHeader.biBitCount + 31) / 32) * 4;
    bf_save.bitmapInfoHeader.biSizeImage = rowSize * bf_save.bitmapInfoHeader.biHeight;

    bf_save.bitmapFileHeader.bfSize = bf_save.bitmapFileHeader.bfOffBits + bf_save.bitmapInfoHeader.biSizeImage;

    bf_save.bitmapInfoHeader.biXPelsPerMeter = 0;
    bf_save.bitmapInfoHeader.biYPelsPerMeter = 0;
    bf_save.bitmapInfoHeader.biClrUsed = 0;
    bf_save.bitmapInfoHeader.biClrImportant = 0;

    // Pixel data (BGR for red)
    bf_save.bitmapData.resize(3); // Direct size, not padded size for this vector
    bf_save.bitmapData[0] = 0;   // Blue
    bf_save.bitmapData[1] = 0;   // Green
    bf_save.bitmapData[2] = 255; // Red

    bf_save.SetValid(); // Mark as valid before saving (Removed true)

    ASSERT_TRUE(bf_save.IsValid());
    EXPECT_TRUE(bf_save.SaveAs(temp_save_as_bmp.c_str()));

    // Verify file exists (basic check)
    std::ifstream file_check(temp_save_as_bmp, std::ios::binary);
    EXPECT_TRUE(file_check.good());
    file_check.close();

    // Test Open() on the saved file
    Bitmap::File bf_reload;
    EXPECT_TRUE(bf_reload.Open(temp_save_as_bmp.c_str()));
    EXPECT_TRUE(bf_reload.IsValid());

    // Compare headers (essential parts)
    EXPECT_EQ(bf_reload.bitmapFileHeader.bfType, bf_save.bitmapFileHeader.bfType);
    EXPECT_EQ(bf_reload.bitmapFileHeader.bfSize, bf_save.bitmapFileHeader.bfSize);
    EXPECT_EQ(bf_reload.bitmapFileHeader.bfOffBits, bf_save.bitmapFileHeader.bfOffBits);

    EXPECT_EQ(bf_reload.bitmapInfoHeader.biSize, bf_save.bitmapInfoHeader.biSize);
    EXPECT_EQ(bf_reload.bitmapInfoHeader.biWidth, bf_save.bitmapInfoHeader.biWidth);
    EXPECT_EQ(bf_reload.bitmapInfoHeader.biHeight, bf_save.bitmapInfoHeader.biHeight);
    EXPECT_EQ(bf_reload.bitmapInfoHeader.biPlanes, bf_save.bitmapInfoHeader.biPlanes);
    EXPECT_EQ(bf_reload.bitmapInfoHeader.biBitCount, bf_save.bitmapInfoHeader.biBitCount);
    EXPECT_EQ(bf_reload.bitmapInfoHeader.biCompression, bf_save.bitmapInfoHeader.biCompression);
    EXPECT_EQ(bf_reload.bitmapInfoHeader.biSizeImage, bf_save.bitmapInfoHeader.biSizeImage);

    // Compare pixel data
    ASSERT_EQ(bf_reload.bitmapData.size(), bf_save.bitmapData.size());
    EXPECT_EQ(bf_reload.bitmapData[0], bf_save.bitmapData[0]); // B
    EXPECT_EQ(bf_reload.bitmapData[1], bf_save.bitmapData[1]); // G
    EXPECT_EQ(bf_reload.bitmapData[2], bf_save.bitmapData[2]); // R

    // Test Save()
    bf_reload.Rename(temp_save_bmp.c_str()); // Rename before Save()
    EXPECT_TRUE(bf_reload.Save());
    std::ifstream file_check_save(temp_save_bmp, std::ios::binary);
    EXPECT_TRUE(file_check_save.good());
    file_check_save.close();
}

// Rename Test
TEST_F(BitmapFileTest, RenameFile) {
    // Create a dummy file to associate with the Bitmap::File object initially
    // Note: Bitmap::File constructor or Open() doesn't create the file if it doesn't exist.
    // For this test, we only care about the internal filename string.
    // No need to actually create "rename_initial.bmp" on disk for this specific test's purpose.
    
    Bitmap::File bf_rename(temp_rename_initial_bmp.c_str());
    EXPECT_EQ(temp_rename_initial_bmp, bf_rename.Filename());

    bf_rename.Rename("rename_final.bmp");
    EXPECT_EQ("rename_final.bmp", bf_rename.Filename());
}

// TODO: Test Open() with an invalid BMP file (e.g., wrong bfType, corrupted headers)
// TODO: Test SaveAs() when the Bitmap::File is not valid
// TODO: Test Save() when the Bitmap::File is not valid or filename is empty

// int main(int argc, char **argv) { // Removed to avoid multiple definitions of main
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }
