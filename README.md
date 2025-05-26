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
