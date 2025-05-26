#include "bitmap/bitmap.h" // Adjust path if necessary, assumes bitmap.h is in src/
#include <iostream>     // For std::cout, std::cerr

int main() {
    Bitmap::File originalBitmap;
    const char* inputFilename = "test.bmp";

    std::cout << "Bitmap Processing Demo" << std::endl;
    std::cout << "----------------------" << std::endl;
    std::cout << "Attempting to load image: " << inputFilename << std::endl;

    if (!originalBitmap.Open(inputFilename)) {
        std::cerr << "********************************************************************************" << std::endl;
        std::cerr << "Error: Could not open '" << inputFilename << "'." << std::endl;
        std::cerr << "Please ensure this file exists in the same directory as the executable." << std::endl;
        std::cerr << "You can copy any BMP image (e.g., from Windows, or download one) and rename it to 'test.bmp'." << std::endl;
        std::cerr << "A common source of BMP files is MS Paint (save as BMP)." << std::endl;
        std::cerr << "********************************************************************************" << std::endl;
        return 1;
    }
    std::cout << "'" << inputFilename << "' loaded successfully." << std::endl << std::endl;

    // --- Example 1: Invert Colors ---
    std::cout << "Processing: Invert Colors..." << std::endl;
    Bitmap::File invertedBitmap = InvertImageColors(originalBitmap);
    if (invertedBitmap.IsValid()) {
        if (invertedBitmap.SaveAs("output_inverted.bmp")) {
            std::cout << "Saved: output_inverted.bmp" << std::endl;
        } else {
            std::cerr << "Error: Failed to save output_inverted.bmp." << std::endl;
        }
    } else {
        std::cerr << "Error: Failed to invert colors." << std::endl;
    }
    std::cout << std::endl;

    // --- Example 2: Apply Sepia Tone to the original ---
    std::cout << "Processing: Apply Sepia Tone..." << std::endl;
    Bitmap::File sepiaBitmap = ApplySepiaTone(originalBitmap);
    if (sepiaBitmap.IsValid()) {
        if (sepiaBitmap.SaveAs("output_sepia.bmp")) {
            std::cout << "Saved: output_sepia.bmp" << std::endl;
        } else {
            std::cerr << "Error: Failed to save output_sepia.bmp." << std::endl;
        }
    } else {
        std::cerr << "Error: Failed to apply sepia tone." << std::endl;
    }
    std::cout << std::endl;

    // --- Example 3: Apply Box Blur and then change contrast ---
    // We'll use a copy of the original for this chain of operations.
    std::cout << "Processing: Box Blur (Radius 2) then Contrast (Factor 1.5)..." << std::endl;
    Bitmap::File processedBitmap = originalBitmap; // Start with a fresh copy for multi-step processing

    processedBitmap = ApplyBoxBlur(processedBitmap, 2); // Radius 2
    if (!processedBitmap.IsValid()) {
         std::cerr << "Error: Failed to apply box blur." << std::endl;
    } else {
        std::cout << "Step 1: Box blur applied." << std::endl;
        
        processedBitmap = ChangeImageContrast(processedBitmap, 1.5f); // Increase contrast
        if (!processedBitmap.IsValid()) {
            std::cerr << "Error: Failed to change contrast after blur." << std::endl;
        } else {
            std::cout << "Step 2: Contrast increased." << std::endl;
            if (processedBitmap.SaveAs("output_blurred_contrasted.bmp")) {
                std::cout << "Saved: output_blurred_contrasted.bmp" << std::endl;
            } else {
                std::cerr << "Error: Failed to save output_blurred_contrasted.bmp." << std::endl;
            }
        }
    }
    std::cout << std::endl;
    
    // --- Example 4: Demonstrate Shrink Image & Grayscale ---
    std::cout << "Processing: Shrink Image (Factor 2) then Grayscale..." << std::endl;
    Bitmap::File shrunkBitmap = originalBitmap; // Start with a fresh copy

    shrunkBitmap = ShrinkImage(shrunkBitmap, 2);
    if (!shrunkBitmap.IsValid()) {
        std::cerr << "Error: Failed to shrink image." << std::endl;
    } else {
        std::cout << "Step 1: Image shrunk." << std::endl;
        shrunkBitmap = GreyscaleImage(shrunkBitmap);
        if(!shrunkBitmap.IsValid()){
            std::cerr << "Error: Failed to apply greyscale after shrinking." << std::endl;
        } else {
            std::cout << "Step 2: Greyscale applied." << std::endl;
            if(shrunkBitmap.SaveAs("output_shrunk_greyscale.bmp")){
                std::cout << "Saved: output_shrunk_greyscale.bmp" << std::endl;
            } else {
                 std::cerr << "Error: Failed to save output_shrunk_greyscale.bmp." << std::endl;
            }
        }
    }
    std::cout << std::endl;

    std::cout << "Main example processing complete. Check for output_*.bmp files." << std::endl;
    return 0;
}