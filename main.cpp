#include "../../include/bitmap.hpp" // Using BmpTool API
#include "../../src/bitmapfile/bitmap_file.h" // For BITMAPFILEHEADER for writeFile logic

#include <iostream>     // For std::cout, std::cerr
#include <vector>       // For std::vector
#include <string>       // For std::string
#include <fstream>      // For std::ifstream, std::ofstream
#include <cstdint>      // For uint8_t etc.
#include <span>         // For std::span
#include <cstring>      // For std::memcpy

// File I/O Utilities
std::vector<uint8_t> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file for reading: " << filename << std::endl;
        return {};
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(size)); // Ensure size_t for vector constructor
    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return buffer;
    }
    std::cerr << "Error: Could not read file: " << filename << std::endl;
    return {};
}

bool writeFile(const std::string& filename, const std::vector<uint8_t>& data) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
        return false;
    }
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    if (!file.good()) {
        std::cerr << "Error: Could not write all data to file: " << filename << std::endl;
        return false;
    }
    return true;
}

// Helper to print BmpTool errors
void printError(BmpTool::BitmapError error, const std::string& operation_name) {
    std::cerr << "Error during " << operation_name << ": " << static_cast<int>(error) << std::endl;
}


int main() {
    const std::string inputFilename = "test.bmp";

    std::cout << "Bitmap Processing Demo with BmpTool API" << std::endl;
    std::cout << "---------------------------------------" << std::endl;
    std::cout << "Attempting to load image: " << inputFilename << std::endl;

    std::vector<uint8_t> file_data = readFile(inputFilename);
    if (file_data.empty()) {
        std::cerr << "********************************************************************************" << std::endl;
        std::cerr << "Error: Could not read '" << inputFilename << "'." << std::endl;
        std::cerr << "Please ensure this file exists in the same directory as the executable." << std::endl;
        std::cerr << "You can copy any BMP image (e.g., from Windows, or download one) and rename it to 'test.bmp'." << std::endl;
        std::cerr << "A common source of BMP files is MS Paint (save as BMP)." << std::endl;
        std::cerr << "********************************************************************************" << std::endl;
        return 1;
    }

    BmpTool::Result<BmpTool::Bitmap, BmpTool::BitmapError> load_result = BmpTool::load(file_data);
    if (!load_result.isSuccess()) {
        printError(load_result.error(), "loading " + inputFilename);
        return 1;
    }
    BmpTool::Bitmap originalBitmap = load_result.value();
    std::cout << "'" << inputFilename << "' loaded successfully. Dimensions: "
              << originalBitmap.w << "x" << originalBitmap.h << std::endl << std::endl;

    BmpTool::Bitmap currentBitmap; // To hold results of operations

    // --- Example 1: Invert Colors ---
    std::cout << "Processing: Invert Colors..." << std::endl;
    auto invert_result = BmpTool::invertColors(originalBitmap);
    if (invert_result.isSuccess()) {
        currentBitmap = invert_result.value();
        std::vector<uint8_t> output_buffer(54 + currentBitmap.w * currentBitmap.h * 4); // Estimate
        auto save_res = BmpTool::save(currentBitmap, output_buffer);
        if (save_res.isSuccess()) {
            BITMAPFILEHEADER fh_out;
            std::memcpy(&fh_out, output_buffer.data(), sizeof(BITMAPFILEHEADER));
            std::vector<uint8_t> actual_data(output_buffer.begin(), output_buffer.begin() + fh_out.bfSize);
            if (writeFile("output_inverted.bmp", actual_data)) {
                std::cout << "Saved: output_inverted.bmp" << std::endl;
            } else {
                 std::cerr << "Error writing output_inverted.bmp" << std::endl;
            }
        } else {
            printError(save_res.error(), "saving inverted image");
        }
    } else {
        printError(invert_result.error(), "inverting colors");
    }
    std::cout << std::endl;

    // --- Example 2: Apply Sepia Tone to the original ---
    std::cout << "Processing: Apply Sepia Tone..." << std::endl;
    auto sepia_result = BmpTool::applySepiaTone(originalBitmap);
    if (sepia_result.isSuccess()) {
        currentBitmap = sepia_result.value();
        std::vector<uint8_t> output_buffer(54 + currentBitmap.w * currentBitmap.h * 4);
        auto save_res = BmpTool::save(currentBitmap, output_buffer);
        if (save_res.isSuccess()) {
            BITMAPFILEHEADER fh_out;
            std::memcpy(&fh_out, output_buffer.data(), sizeof(BITMAPFILEHEADER));
            std::vector<uint8_t> actual_data(output_buffer.begin(), output_buffer.begin() + fh_out.bfSize);
            if (writeFile("output_sepia.bmp", actual_data)) {
                std::cout << "Saved: output_sepia.bmp" << std::endl;
            } else {
                std::cerr << "Error writing output_sepia.bmp" << std::endl;
            }
        } else {
            printError(save_res.error(), "saving sepia image");
        }
    } else {
        printError(sepia_result.error(), "applying sepia tone");
    }
    std::cout << std::endl;

    // --- Example 3: Apply Box Blur and then change contrast ---
    std::cout << "Processing: Box Blur (Radius 2) then Contrast (Factor 1.5)..." << std::endl;
    // currentBitmap = originalBitmap; // This would make a shallow copy if Bitmap struct is not careful.
                                     // For safety, let's reload or re-assign from originalBitmap if we need a pristine copy.
                                     // Or ensure Bitmap struct has proper copy semantics if it manages its own data deeply.
                                     // Given BmpTool::Bitmap is a struct with std::vector, it has deep copy semantics.
    BmpTool::Bitmap tempProcessedBitmap = originalBitmap;


    auto blur_result = BmpTool::applyBoxBlur(tempProcessedBitmap, 2);
    if (!blur_result.isSuccess()) {
         printError(blur_result.error(), "applying box blur");
    } else {
        tempProcessedBitmap = blur_result.value();
        std::cout << "Step 1: Box blur applied." << std::endl;
        
        auto contrast_result = BmpTool::changeContrast(tempProcessedBitmap, 1.5f);
        if (!contrast_result.isSuccess()) {
            printError(contrast_result.error(), "changing contrast after blur");
        } else {
            tempProcessedBitmap = contrast_result.value();
            std::cout << "Step 2: Contrast increased." << std::endl;

            std::vector<uint8_t> output_buffer(54 + tempProcessedBitmap.w * tempProcessedBitmap.h * 4);
            auto save_res = BmpTool::save(tempProcessedBitmap, output_buffer);
            if (save_res.isSuccess()) {
                BITMAPFILEHEADER fh_out;
                std::memcpy(&fh_out, output_buffer.data(), sizeof(BITMAPFILEHEADER));
                std::vector<uint8_t> actual_data(output_buffer.begin(), output_buffer.begin() + fh_out.bfSize);
                if (writeFile("output_blurred_contrasted.bmp", actual_data)) {
                    std::cout << "Saved: output_blurred_contrasted.bmp" << std::endl;
                } else {
                    std::cerr << "Error writing output_blurred_contrasted.bmp" << std::endl;
                }
            } else {
                printError(save_res.error(), "saving blurred_contrasted image");
            }
        }
    }
    std::cout << std::endl;
    
    // --- Example 4: Demonstrate Shrink Image & Grayscale ---
    std::cout << "Processing: Shrink Image (Factor 2) then Grayscale..." << std::endl;
    tempProcessedBitmap = originalBitmap; // Start with a fresh copy

    auto shrink_result = BmpTool::shrink(tempProcessedBitmap, 2);
    if (!shrink_result.isSuccess()) {
        printError(shrink_result.error(), "shrinking image");
    } else {
        tempProcessedBitmap = shrink_result.value();
        std::cout << "Step 1: Image shrunk." << std::endl;

        if (tempProcessedBitmap.w == 0 || tempProcessedBitmap.h == 0) {
            std::cout << "Image shrunk to zero dimensions, skipping further processing and save." << std::endl;
        } else {
            auto greyscale_result = BmpTool::greyscale(tempProcessedBitmap);
            if(!greyscale_result.isSuccess()){
                printError(greyscale_result.error(), "applying greyscale after shrinking");
            } else {
                tempProcessedBitmap = greyscale_result.value();
                std::cout << "Step 2: Greyscale applied." << std::endl;

                std::vector<uint8_t> output_buffer(54 + tempProcessedBitmap.w * tempProcessedBitmap.h * 4);
                auto save_res = BmpTool::save(tempProcessedBitmap, output_buffer);
                if (save_res.isSuccess()) {
                    BITMAPFILEHEADER fh_out;
                    std::memcpy(&fh_out, output_buffer.data(), sizeof(BITMAPFILEHEADER));
                    std::vector<uint8_t> actual_data(output_buffer.begin(), output_buffer.begin() + fh_out.bfSize);
                    if(writeFile("output_shrunk_greyscale.bmp", actual_data)){
                        std::cout << "Saved: output_shrunk_greyscale.bmp" << std::endl;
                    } else {
                        std::cerr << "Error writing output_shrunk_greyscale.bmp" << std::endl;
                    }
                } else {
                    printError(save_res.error(), "saving shrunk_greyscale image");
                }
            }
        }
    }
    std::cout << std::endl;

    std::cout << "Main example processing complete. Check for output_*.bmp files." << std::endl;
    return 0;
}