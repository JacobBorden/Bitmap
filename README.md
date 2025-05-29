# C++ Bitmap Image Manipulation Library

A simple C++ library for performing various image manipulation operations on BMP (Bitmap) files. 
This library currently focuses on pixel-level adjustments and basic geometric transformations.

## Features

The library supports the following image manipulation functions:

*   **Color Adjustments:**
    *   Greyscale Conversion
    *   Brightness Adjustment (Overall and per R,G,B,Magenta,Yellow,Cyan channel)
    *   Contrast Adjustment (Overall and per R,G,B,Magenta,Yellow,Cyan channel)
    *   Saturation Adjustment (Overall and per R,G,B,Magenta,Yellow,Cyan channel)
    *   Luminance Adjustment (per R,G,B,Magenta,Yellow,Cyan channel)
    *   Invert Colors
    *   Sepia Tone
*   **Effects & Filters:**
    *   Box Blur
*   **Geometric Transformations:**
    *   Shrink Image (reduce size)
    *   Rotate Image (Clockwise and Counter-Clockwise)
    *   Mirror Image (horizontal flip)
    *   Flip Image (vertical flip)
*   **Utility:**
    *   Screen Capture (Windows only: `ScreenShotWindow`)
    *   Load BMP from file
    *   Save BMP to file

## Dependencies

This library utilizes:
*   A simple Matrix library (from `src/matrix`)
*   A BMP file handling library (from `src/bitmapfile`)
These are now part of the main source tree under the `src/` directory.

## Core Modules
*   **Bitmap Library (`src/bitmap`)**: Provides core image processing functions.
*   **Bitmap File Handler (`src/bitmapfile`)**: Handles loading and saving of BMP files.
*   **Matrix Library (`src/matrix`)**: A generic matrix manipulation library used by the bitmap processing functions.

## New Span-Based API (`BmpTool`)

For more direct memory-based operations and a stable interface, the `BmpTool` API is provided.

The main header for this API is `include/bitmap.hpp`.

Core components include:
*   `BmpTool::Bitmap`: A struct holding image dimensions (width, height, bits-per-pixel) and a `std::vector<uint8_t>` for RGBA pixel data.
*   `BmpTool::load()`: Loads BMP data from a `std::span<const uint8_t>` into a `BmpTool::Bitmap`.
*   `BmpTool::save()`: Saves a `BmpTool::Bitmap` to a `std::span<uint8_t>`.
*   `BmpTool::Result<T, E>`: Used for functions that can return a value or an error, with `BmpTool::BitmapError` providing specific error codes.

### `BmpTool` API Usage Example

```cpp
#include "include/bitmap.hpp" // Public API
#include <vector>
#include <fstream> // For reading/writing files to/from buffer for example
#include <iostream>

// Helper to read a file into a vector
std::vector<uint8_t> read_file_to_buffer(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) return {};
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return buffer;
    }
    return {};
}

// Helper to write a buffer to a file
bool write_buffer_to_file(const std::string& filepath, const std::vector<uint8_t>& buffer, size_t actual_size) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file) return false;
    file.write(reinterpret_cast<const char*>(buffer.data()), actual_size);
    return file.good();
}

int main() {
    // Load an existing BMP into a buffer
    std::vector<uint8_t> input_bmp_data = read_file_to_buffer("input.bmp");
    if (input_bmp_data.empty()) {
        std::cerr << "Failed to read input.bmp into buffer." << std::endl;
        return 1;
    }

    // Use BmpTool::load
    BmpTool::Result<BmpTool::Bitmap, BmpTool::BitmapError> load_result = BmpTool::load(input_bmp_data);

    if (load_result.isError()) {
        std::cerr << "BmpTool::load failed: " << static_cast<int>(load_result.error()) << std::endl;
        return 1;
    }
    BmpTool::Bitmap my_bitmap = load_result.value();
    std::cout << "Loaded input.bmp into BmpTool::Bitmap: "
              << my_bitmap.w << "x" << my_bitmap.h << " @ " << my_bitmap.bpp << "bpp" << std::endl;

    // (Perform some manipulation on my_bitmap.data if desired)
    // For example, invert the red channel for all pixels:
    // for (size_t i = 0; i < my_bitmap.data.size(); i += 4) {
    //     my_bitmap.data[i] = 255 - my_bitmap.data[i]; // Invert Red
    // }

    // Prepare a buffer for saving
    // Estimate required size: headers (54) + data (W*H*4)
    size_t estimated_output_size = 54 + my_bitmap.w * my_bitmap.h * 4;
    std::vector<uint8_t> output_bmp_buffer(estimated_output_size);

    // Use BmpTool::save
    BmpTool::Result<void, BmpTool::BitmapError> save_result = BmpTool::save(my_bitmap, output_bmp_buffer);

    if (save_result.isError()) { // For Result<void,E>, isError() or checking error() != E::Ok
        std::cerr << "BmpTool::save failed: " << static_cast<int>(save_result.error()) << std::endl;
        return 1;
    }
    std::cout << "BmpTool::Bitmap saved to buffer." << std::endl;

    // To get the actual size of the BMP written to the buffer (needed for writing to file):
    // One way is to read bfSize from the header in output_bmp_buffer
    // For example:
    // #include "src/bitmapfile/bitmap_file.h" // For BITMAPFILEHEADER definition
    // BITMAPFILEHEADER* fh = reinterpret_cast<BITMAPFILEHEADER*>(output_bmp_buffer.data());
    // size_t actual_written_size = fh->bfSize;
    // This requires including the BITMAPFILEHEADER definition.
    // The save function itself doesn't return it, so this is a known aspect of the API.
    // For this example, we'll use estimated_output_size, but actual_written_size is more robust.
    // A more robust approach would be to parse bfSize or ensure save guarantees fitting within the span and updating its size.
    // For this example, we assume the buffer is large enough and we write what's estimated.
    // If the actual BMP is smaller, this might write extra uninitialized bytes from the buffer.
    // If actual is larger (should not happen with correct estimation and save), it's a problem.
    // The best is to parse bfSize from output_bmp_buffer.data().

    if (write_buffer_to_file("output_new_api.bmp", output_bmp_buffer, estimated_output_size /* ideally actual_written_size from parsed header */)) {
        std::cout << "Output buffer saved to output_new_api.bmp" << std::endl;
    } else {
        std::cerr << "Failed to write output_new_api.bmp." << std::endl;
    }

    return 0;
}
```

## Building the Project

The project uses CMake for building.

```bash
# Create a build directory
mkdir build
cd build

# Configure the project
cmake ..

# Build the library (if configured as a library) and executables (like main example and tests)
make 
# or use your specific build system command e.g., mingw32-make

# Run tests (if configured)
ctest 
# or directly run the test executable: ./bitmap_tests (or tests\bitmap_tests.exe on Windows)
```

## Basic Usage Example

```cpp
#include "bitmap/bitmap.h" // Adjust path if necessary, assumes src/ is an include dir
#include <iostream>

int main() {
    Bitmap::File myBitmap;

    // Load an image
    if (!myBitmap.Open("input.bmp")) {
        std::cerr << "Error opening input.bmp" << std::endl;
        return 1;
    }

    // Apply some manipulations
    myBitmap = GreyscaleImage(myBitmap);
    myBitmap = ChangeImageBrightness(myBitmap, 1.2f); // Increase brightness by 20%
    myBitmap = ApplyBoxBlur(myBitmap, 2); // Apply box blur with radius 2

    // Save the result
    if (!myBitmap.SaveAs("output.bmp")) {
        std::cerr << "Error saving output.bmp" << std::endl;
        return 1;
    }

    std::cout << "Image processed and saved as output.bmp" << std::endl;
    return 0;
}
```
Refer to `main.cpp` for more examples.

## Fuzz Testing

This project includes a suite of fuzz tests to help ensure code robustness and identify potential vulnerabilities. The fuzzing setup uses Clang's libFuzzer along with AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan).

### Enabling Fuzzing

To build the fuzz targets, enable the `ENABLE_FUZZING` option when configuring with CMake:

```bash
cmake -S . -B build_fuzz -DENABLE_FUZZING=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build_fuzz --config Debug
```
This requires Clang to be installed and set as the C++ compiler. The GitHub Actions workflow (`.github/workflows/fuzzing.yml`) does this automatically.

### Available Fuzz Targets

The following fuzz targets are available and will be built when fuzzing is enabled:

*   `fuzz_bitmap`: Tests the `BmpTool::load` function from `include/bitmap.hpp`.
*   `fuzz_bmp_tool_save`: Tests the `BmpTool::save` function from `include/bitmap.hpp`.
*   `fuzz_bitmap_file`: Tests operations of the `Bitmap::File` class from `src/bitmapfile/bitmap_file.h`.
*   `fuzz_image_operations`: Tests various image manipulation functions from `src/bitmap/bitmap.h`.
*   `fuzz_matrix`: Tests operations of the `Matrix::Matrix` class from `src/matrix/matrix.h`.

Each fuzzer will run for a short duration (e.g., 60 seconds) when executed via the GitHub Actions workflow. They maintain their own corpus directories within `build_fuzz/corpus_fuzzing/`.
