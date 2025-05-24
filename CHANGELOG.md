# Changelog

All notable changes to this project will be documented in this file.

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
