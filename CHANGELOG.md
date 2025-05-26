# Changelog

All notable changes to this project will be documented in this file.

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
