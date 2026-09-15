# Changelog

All notable changes to this project will be documented in this file.

## [0.3.0] - 2025-05-28

### Added
- **New Bitmap API Layer (`BmpTool`)**:
    - Introduced a new public API header `include/bitmap.hpp`.
    - Added `BmpTool::load(std::span<const uint8_t>)` function to load BMP data from a memory span into a `BmpTool::Bitmap` (RGBA format). This function bridges to the existing library's `::CreateMatrixFromBitmap` after parsing the input span.
    - Added `BmpTool::save(const BmpTool::Bitmap&, std::span<uint8_t>)` function to save a `BmpTool::Bitmap` (RGBA) to a memory span. This function bridges to the existing library's `::CreateBitmapFromMatrix` and then serializes the resulting `::Bitmap::File` to the span.
    - Defined `BmpTool::Bitmap` struct for RGBA pixel data and `BmpTool::Result` for error handling.
    - Implemented the bridging logic in `src/format/bitmap.cpp`.
    - Added Doxygen comments for the new public API.
- **API Roundtrip Test**:
    - Added `tests/api_roundtrip.cpp` to verify that loading, saving, and re-loading a bitmap using the new API results in identical data.
- **Build System Updates**:
    - Updated CMakeLists.txt files (root and tests) to include the new API implementation and test.
    - Set C++ standard to C++20 globally in the root CMakeLists.txt.

### Changed
- The `bitmap` library now exposes the `BmpTool` API via `include/bitmap.hpp` for simplified bitmap operations.
- **Documentation**:
    - Reviewed and significantly updated `README.md` for accuracy regarding features, API examples (old vs. new), build instructions, and added "Code Documentation" and "Contributing" sections.
    - Reviewed and extensively updated `src/matrix/Documentation/Matrix.MD` to align with the current `src/matrix/matrix.h` implementation, including documenting move semantics, new functions, and correcting outdated information.

## [0.2.0] - 2024-07-27

### Changed
- Reorganized project structure: moved `bitmap.h` and `bitmap.cpp` to `src/bitmap/`. Updated include paths and CMakeLists.txt accordingly.
- Integrated `matrix` and `bitmapfile` modules directly into `src/` instead of a separate `dependencies` folder.

### Added
- Comprehensive unit tests for the `Bitmap::File` module (`tests/test_bitmap_file.cpp`), covering constructors, file operations (open, save, saveas), and validation logic.
- Comprehensive unit tests for the `Matrix` module (`tests/test_matrix.cpp`), covering constructors, arithmetic operations, manipulations (transpose, inverse, determinant), and merge/split functionality.

### Fixed
- Corrected include paths in various files to reflect the new project structure.
- Updated CMakeLists.txt files to correctly locate and build all source files and tests under the new structure.

## [0.1.0] - 2025-05-23

### Added
- New image manipulation functions:
    - `InvertImageColors` / `InvertPixelColor`
    - `ApplySepiaTone` / `ApplySepiaToPixel`
    - `ApplyBoxBlur`
- Comprehensive unit testing framework (`tests/test_bitmap.cpp`) using basic assertions.
- Unit tests for most pixel-level functions (Greyscale, Invert, Sepia, Brightness, Contrast, Saturation, Luminance - including color-specific variants).
- Unit tests for image-level functions (`ApplyBoxBlur`, `ShrinkImage`, `RotateImageCounterClockwise`, and basic checks for others).
- This `README.md` and `CHANGELOG.md` file.

### Changed
- Refined contrast adjustment logic for secondary colors (`ChangePixelContrastMagenta`, `ChangePixelContrastYellow`, `ChangePixelContrastCyan`) to apply contrast to constituent primary channels directly.
- Improved inline code documentation (comments) in `src/bitmap.h` and `src/bitmap.cpp` for clarity and maintainability.
- Standardized clamping of pixel component values (0-255) in various functions.
- Corrected `std::min` usage in saturation and luminance functions.

### Fixed
- Corrected syntax errors in the original implementations of `ChangePixelContrastMagenta` and `ChangePixelContrastCyan`.
- Corrected variable name usage (`magenta` vs `cyan`) in `ChangePixelContrastCyan`.
- Improved GDI resource management in `ScreenShotWindow` by ensuring `DeleteDC`, `ReleaseDC`, and `DeleteObject` are called.
- Corrected a typo in `ChangePixelSaturationBlue` condition.
- Corrected a typo in the `ChangePixelBrightness` parameter name in `src/bitmap.h`.
