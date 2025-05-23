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
*   A simple Matrix library (from `dependencies/matrix`)
*   A BMP file handling library (from `dependencies/bitmapfile`)
These are included in the `dependencies` directory.

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
#include "src/bitmap.h" // Adjust path if necessary
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
