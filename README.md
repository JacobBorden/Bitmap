# C++ Bitmap Image Manipulation Library

A simple C++ library for performing various image manipulation operations on BMP (Bitmap) files. 
This library currently focuses on pixel-level adjustments and basic geometric transformations.

## Features

The library supports the following image manipulation functions:

*   **Color Adjustments:**
    *   Greyscale Conversion
    *   Brightness Adjustment (Overall)
    *   Contrast Adjustment (Overall)
    *   Saturation Adjustment (Overall and per R,G,B,Magenta,Yellow,Cyan channel)
    *   Luminance Adjustment (per R,G,B,Magenta,Yellow,Cyan channel)
    *   Invert Colors
    *   Sepia Tone
*   **Effects & Filters:**
    *   Box Blur
*   **Geometric Transformations:**
    *   Shrink Image (reduce size)
    *   Rotate Image (90 degrees Clockwise and Counter-Clockwise)
    *   Mirror Image (horizontal flip)
    *   Flip Image (vertical flip)
*   **Utility (Old API - `src/bitmap/bitmap.h`):**
    *   Screen Capture (Windows only: `ScreenShotWindow`)
    *   Load BMP from file (`Bitmap::File::Open`)
    *   Save BMP to file (`Bitmap::File::SaveAs`)
*   **Utility (New API - `include/bitmap.hpp`):**
    *   Load BMP from memory (`BmpTool::load`)
    *   Save BMP to memory (`BmpTool::save`)

Note: The old API (`src/bitmap/bitmap.h` and `src/bitmapfile/bitmap_file.h`) provides file-based loading/saving and additional pixel-level operations. The new `BmpTool` API (`include/bitmap.hpp`) focuses on memory span-based operations and provides a more modern interface for whole-image manipulations.

## Dependencies

This library has internal dependencies:
*   A simple Matrix library (from `src/matrix`) used by the old API.
*   BMP file handling structures (from `src/bitmapfile`) used by both APIs.
These are part of the main source tree under the `src/` directory. No external libraries are required for core functionality.

## Core Modules
*   **New Bitmap API (`include/bitmap.hpp`)**: Provides the `BmpTool` namespace for modern, span-based image processing. This is the recommended API for new projects.
*   **Old Bitmap Library (`src/bitmap`)**: Provides older image processing functions using the `Bitmap::File` class.
*   **Bitmap File Handler (`src/bitmapfile`)**: Contains structures and functions for handling BMP file headers and low-level data, used by both APIs.
*   **Matrix Library (`src/matrix`)**: A generic matrix manipulation library, primarily used by the old bitmap library.

## New Span-Based API (`BmpTool`)

For more direct memory-based operations and a stable interface, the `BmpTool` API is provided. This API operates on image data in memory buffers (`std::span`).

The main header for this API is `include/bitmap.hpp`.

Core components include:
*   `BmpTool::Bitmap`: A struct holding image dimensions (width, height, bits-per-pixel) and a `std::vector<uint8_t>` for pixel data (typically RGBA).
*   `BmpTool::load(std::span<const uint8_t> bmp_data)`: Loads BMP data from a memory span into a `BmpTool::Bitmap`.
*   `BmpTool::save(const BmpTool::Bitmap& bitmap, std::span<uint8_t> out_bmp_buffer)`: Saves a `BmpTool::Bitmap` to a memory span. The size of the output BMP data can be determined from the `bfSize` field of the `BITMAPFILEHEADER` written to the beginning of the `out_bmp_buffer`.
*   `BmpTool::Result<T, BmpTool::BitmapError>`: Used by functions to return either a value `T` or a `BmpTool::BitmapError` enum, indicating the outcome of the operation. For functions that don't return a value on success (like `save`), `BmpTool::Result<void, BmpTool::BitmapError>` is used (internally represented as `Result<Success, BitmapError>`).

All image manipulation functions in the `BmpTool` namespace (e.g., `BmpTool::greyscale`, `BmpTool::shrink`) take a `const BmpTool::Bitmap&` as input and return a `BmpTool::Result<BmpTool::Bitmap, BmpTool::BitmapError>`.

### `BmpTool` API Usage Example

The following example demonstrates loading a BMP file into a buffer, processing it using `BmpTool`, and saving the result. This example is a simplified version of `main.cpp`.

```cpp
#include "include/bitmap.hpp" // Public API for BmpTool
#include "src/bitmapfile/bitmap_file.h" // For BITMAPFILEHEADER definition (to get actual size)

#include <vector>
#include <fstream>    // For std::ifstream, std::ofstream
#include <iostream>   // For std::cerr, std::cout
#include <cstring>    // For std::memcpy if reading header manually

// Helper to read a file into a vector
std::vector<uint8_t> read_file_to_buffer(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open " << filepath << std::endl;
        return {};
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return buffer;
    }
    std::cerr << "Failed to read " << filepath << std::endl;
    return {};
}

// Helper to write a buffer of a specific size to a file
bool write_buffer_to_file(const std::string& filepath, const std::vector<uint8_t>& buffer, size_t actual_size) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open " << filepath << " for writing." << std::endl;
        return false;
    }
    file.write(reinterpret_cast<const char*>(buffer.data()), actual_size);
    return file.good();
}

int main() {
    const std::string input_filename = "input.bmp";
    const std::string output_filename = "output_processed.bmp";

    // 1. Load an existing BMP into a buffer
    std::vector<uint8_t> input_bmp_data = read_file_to_buffer(input_filename);
    if (input_bmp_data.empty()) {
        return 1;
    }

    // 2. Use BmpTool::load to parse the buffer
    BmpTool::Result<BmpTool::Bitmap, BmpTool::BitmapError> load_result = BmpTool::load(input_bmp_data);

    if (load_result.isError()) {
        std::cerr << "BmpTool::load failed: " << static_cast<int>(load_result.error()) << std::endl;
        return 1;
    }
    BmpTool::Bitmap my_bitmap = load_result.value();
    std::cout << "Loaded '" << input_filename << "': "
              << my_bitmap.w << "x" << my_bitmap.h << " @ " << my_bitmap.bpp << "bpp" << std::endl;

    // 3. Perform some manipulation (e.g., greyscale)
    BmpTool::Result<BmpTool::Bitmap, BmpTool::BitmapError> greyscale_result = BmpTool::greyscale(my_bitmap);
    if(greyscale_result.isError()){
        std::cerr << "BmpTool::greyscale failed: " << static_cast<int>(greyscale_result.error()) << std::endl;
        return 1;
    }
    my_bitmap = greyscale_result.value(); // Update bitmap with processed one
    std::cout << "Applied greyscale." << std::endl;

    // 4. Prepare a buffer for saving. It must be large enough.
    // Max possible size: BMP headers (54 bytes) + data (Width * Height * 4 bytes/pixel for 32bpp RGBA)
    size_t max_output_size = 54 + my_bitmap.w * my_bitmap.h * 4;
    std::vector<uint8_t> output_bmp_buffer(max_output_size);

    // 5. Use BmpTool::save
    BmpTool::Result<void, BmpTool::BitmapError> save_result = BmpTool::save(my_bitmap, output_bmp_buffer);

    if (save_result.isError()) {
        std::cerr << "BmpTool::save failed: " << static_cast<int>(save_result.error()) << std::endl;
        return 1;
    }
    std::cout << "BmpTool::Bitmap saved to output buffer." << std::endl;

    // 6. Determine actual size of BMP written to buffer and write to file
    // The BMP File Header (BITMAPFILEHEADER) is at the start of the buffer.
    // Its bfSize field contains the total size of the BMP file.
    BITMAPFILEHEADER* fh = reinterpret_cast<BITMAPFILEHEADER*>(output_bmp_buffer.data());
    size_t actual_written_size = fh->bfSize;

    // Ensure actual_written_size is not greater than our buffer to prevent reading out of bounds.
    if (actual_written_size > max_output_size) {
        std::cerr << "Error: saved BMP size (" << actual_written_size
                  << ") exceeds buffer capacity (" << max_output_size << ")." << std::endl;
        return 1;
    }

    if (write_buffer_to_file(output_filename, output_bmp_buffer, actual_written_size)) {
        std::cout << "Processed image saved to '" << output_filename << "'" << std::endl;
    } else {
        std::cerr << "Failed to write output to '" << output_filename << "'." << std::endl;
        return 1;
    }

    return 0;
}
```

## Building the Project

The project uses CMake for building. Ensure you have CMake (version 3.10 or newer) and a C++20 compatible compiler installed.

```bash
# Create a build directory (out-of-source build is recommended)
mkdir build
cd build

# Configure the project (from the build directory)
cmake ..

# Build the library and executables (e.g., testexe, tests)
# On Linux/macOS with Makefiles (default)
make 
# On Windows with Visual Studio, after cmake .. you might open the .sln or use:
# cmake --build . --config Release
# For MinGW users:
# mingw32-make

# Run tests (if configured and built)
# From the build directory
ctest 
# or directly run the test executable (e.g., ./tests/bitmap_tests or build\tests\Debug\bitmap_tests.exe)
```

## Basic Usage Example (Old API)

The following example demonstrates basic usage of the older, file-based API found in `src/bitmap/bitmap.h` and `src/bitmapfile/bitmap_file.h`.

```cpp
#include "bitmap/bitmap.h" // Old API (assumes src/ is an include directory or path adjusted)
#include <iostream>

int main() {
    Bitmap::File myBitmap; // Uses the old Bitmap::File class

    // Load an image directly from a file path
    if (!myBitmap.Open("input.bmp")) {
        std::cerr << "Error opening input.bmp using old API" << std::endl;
        return 1;
    }
    std::cout << "Loaded input.bmp using old API." << std::endl;

    // Apply some manipulations (old API functions often return a new Bitmap::File)
    Bitmap::File processedBitmap = GreyscaleImage(myBitmap);
    processedBitmap = ChangeImageBrightness(processedBitmap, 1.2f); // Increase brightness by 20%
    processedBitmap = ApplyBoxBlur(processedBitmap, 2); // Apply box blur with radius 2
    std::cout << "Applied manipulations using old API." << std::endl;

    // Save the result to a new file path
    if (!processedBitmap.SaveAs("output_old_api.bmp")) {
        std::cerr << "Error saving output_old_api.bmp using old API" << std::endl;
        return 1;
    }

    std::cout << "Image processed and saved as output_old_api.bmp using old API" << std::endl;
    return 0;
}
```
The `main.cpp` in the root of the repository demonstrates usage of the **new `BmpTool` API**.

## Code Documentation

The public API for `BmpTool` in `include/bitmap.hpp` is documented using Doxygen-style comments.

If Doxygen is installed, you can generate the full HTML documentation by:
1.  Installing Doxygen.
2.  Creating a Doxyfile configuration (e.g., `doxygen -g Doxyfile` in the project root).
3.  Customizing the `Doxyfile` (e.g., set `INPUT` to `include/`, `RECURSIVE` to `YES`).
4.  Running `doxygen Doxyfile` from the project root.
The documentation will then be available in the specified output directory (e.g., `html/`).

## Contributing

Contributions to this project are welcome! Please follow these basic guidelines:

1.  **Fork the repository:** Create your own fork of the project on GitHub.
2.  **Create a branch:** Make a new branch in your fork for your feature or bug fix (e.g., `feature/new-filter` or `fix/brightness-bug`).
3.  **Make your changes:** Implement your changes, ensuring code is clear and follows existing style where possible.
4.  **Add tests:** If you're adding a new feature or fixing a bug, please add appropriate unit tests in the `tests/` directory. Ensure all tests pass by running `ctest` or the test executable.
5.  **Commit your changes:** Make clear, concise commit messages.
6.  **Push to your fork:** Push your changes to your branch on your fork.
7.  **Submit a Pull Request (PR):** Open a PR from your branch to the main repository's `main` branch. Provide a clear description of your changes in the PR.

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
