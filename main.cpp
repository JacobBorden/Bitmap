#include "../../include/bitmap.hpp" // Using BmpTool API

#include <iostream>     // For std::cout, std::cerr
#include <vector>       // For std::vector (used by BmpTool::Bitmap)
#include <string>       // For std::string
#include <cstdint>      // For uint8_t etc. (used by BmpTool::Bitmap)

// Helper to print BmpTool errors
void printError(BmpTool::BitmapError error, const std::string& operation_name) {
    std::cerr << "Error during " << operation_name << ": " << static_cast<int>(error) << std::endl;
}


int main() {
    const std::string inputFilename = "test.bmp";

    std::cout << "Bitmap Processing Demo with BmpTool API" << std::endl;
    std::cout << "---------------------------------------" << std::endl;
    std::cout << "Attempting to load image: " << inputFilename << std::endl;

    BmpTool::Result<BmpTool::Bitmap, BmpTool::BitmapError> load_result = BmpTool::load(inputFilename);
    if (!load_result.isSuccess()) {
        std::cerr << "********************************************************************************" << std::endl;
        std::cerr << "Error: Could not load '" << inputFilename << "' using BmpTool::load." << std::endl;
        std::cerr << "Please ensure this file exists in the same directory as the executable and is a valid BMP." << std::endl;
        std::cerr << "You can copy any BMP image (e.g., from Windows, or download one) and rename it to 'test.bmp'." << std::endl;
        std::cerr << "A common source of BMP files is MS Paint (save as BMP)." << std::endl;
        std::cerr << "********************************************************************************" << std::endl;
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
        auto save_res_file = BmpTool::save(currentBitmap, "output_inverted.bmp");
        if (save_res_file.isSuccess()) {
            std::cout << "Saved: output_inverted.bmp" << std::endl;
        } else {
            std::cerr << "Error writing output_inverted.bmp directly to file." << std::endl;
            printError(save_res_file.error(), "saving inverted image");
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
        auto save_res_file = BmpTool::save(currentBitmap, "output_sepia.bmp");
        if (save_res_file.isSuccess()) {
            std::cout << "Saved: output_sepia.bmp" << std::endl;
        } else {
            std::cerr << "Error writing output_sepia.bmp directly to file." << std::endl;
            printError(save_res_file.error(), "saving sepia image");
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

            auto save_res_file = BmpTool::save(tempProcessedBitmap, "output_blurred_contrasted.bmp");
            if (save_res_file.isSuccess()) {
                std::cout << "Saved: output_blurred_contrasted.bmp" << std::endl;
            } else {
                std::cerr << "Error writing output_blurred_contrasted.bmp directly to file." << std::endl;
                printError(save_res_file.error(), "saving blurred_contrasted image");
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

                auto save_res_file = BmpTool::save(tempProcessedBitmap, "output_shrunk_greyscale.bmp");
                if (save_res_file.isSuccess()) {
                    std::cout << "Saved: output_shrunk_greyscale.bmp" << std::endl;
                } else {
                    std::cerr << "Error writing output_shrunk_greyscale.bmp directly to file." << std::endl;
                    printError(save_res_file.error(), "saving shrunk_greyscale image");
                }
            }
        }
    }
    std::cout << std::endl;

    std::cout << "Main example processing complete. Check for output_*.bmp files." << std::endl;
    return 0;
}