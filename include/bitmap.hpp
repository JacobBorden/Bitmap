#pragma once // Use pragma once for include guard

#include <cstdint> // For uint32_t, uint8_t
#include <vector>   // For std::vector
#include <variant>  // For std::variant in Result, std::monostate
#include <string>   // For error messages if needed (though enum is primary)
#include <span>     // For std::span (will be used by load/save)
#include <stdexcept> // For std::runtime_error, std::bad_variant_access

namespace BmpTool {

/**
 * @brief Represents errors that can occur during Bitmap operations.
 */
enum class BitmapError {
    Ok,                   ///< Operation completed successfully. (Note: Used by save for Result<void,E>)
    InvalidFileHeader,    ///< The BITMAPFILEHEADER is invalid or corrupted.
    InvalidImageHeader,   ///< The BITMAPINFOHEADER is invalid or corrupted.
    UnsupportedBpp,       ///< The bits-per-pixel value is not supported (e.g., not 24 or 32).
    InvalidImageData,     ///< The pixel data is corrupt or inconsistent with header information.
    OutputBufferTooSmall, ///< The provided output buffer (e.g., for saving) is too small.
    IoError,              ///< A generic error occurred during an I/O operation (e.g., reading/writing data).
    NotABmp,              ///< The file is not a BMP file (e.g., magic identifier is incorrect).
    UnknownError,         ///< An unspecified error occurred.
    PayloadTruncated,     ///< The pixel data buffer is truncated or smaller than expected.
    UnsupportedCompression, ///< The compression format is not supported (only uncompressed BI_RGB is supported).
    DimensionOverflow,    ///< Arithmetic overflow occurred while calculating image dimensions or offsets.
    ExceedsMaxDimensions, ///< Image dimensions exceed MAX_SAFE_DIMENSION (65,536 pixels).
    InvalidColorDepth     ///< Color depth is invalid or unsupported (only 24bpp and 32bpp supported).
};

/**
 * @brief A placeholder type to represent a successful operation 
 *        when a Result<void, E> is needed.
 */
struct Success {}; 

/**
 * @brief A template class to represent a result that can either be a value or an error.
 *
 * @tparam T The type of the value if the operation is successful.
 * @tparam E The type of the error if the operation fails.
 */
template <typename T, typename E>
class Result {
public:
    /**
     * @brief Constructs a Result with a success value.
     * @param value The value to store.
     */
    Result(T value) : m_data(value) {}

    /**
     * @brief Constructs a Result with an error.
     * @param error The error to store.
     */
    Result(E error) : m_data(error) {}

    /**
     * @brief Checks if the Result holds a success value.
     * @return True if it holds a value, false otherwise.
     */
    bool isSuccess() const {
        return std::holds_alternative<T>(m_data);
    }

    /**
     * @brief Checks if the Result holds an error.
     * @return True if it holds an error, false otherwise.
     */
    bool isError() const {
        return std::holds_alternative<E>(m_data);
    }

    /**
     * @brief Gets the success value.
     * @return The stored value.
     * @throw std::runtime_error if the Result holds an error or variant is valueless.
     */
    T value() const {
        if (!isSuccess()) { // Check if it's NOT a success
            throw std::runtime_error("Attempted to access value from an error Result or variant holds unexpected type.");
        }
        // isSuccess() being true implies std::holds_alternative<T>(m_data) is true.
        return std::get<T>(m_data);
    }

    /**
     * @brief Gets the error.
     * @return The stored error.
     * @throw std::runtime_error if the Result holds a success value or variant is valueless.
     */
    E error() const {
        if (!isError()) { // Check if it's NOT an error
            throw std::runtime_error("Attempted to access error from a success Result or variant holds unexpected type.");
        }
        // isError() being true implies std::holds_alternative<E>(m_data) is true.
        return std::get<E>(m_data);
    }

    // Operator bool for easy checking like if(result)
    explicit operator bool() const {
        return isSuccess();
    }

private:
    std::variant<T, E> m_data;
};


/**
 * @brief Specialization of Result for operations that return void on success.
 * @tparam E The error type.
 */
template <typename E>
class Result<void, E> {
public:
    /**
     * @brief Constructs a success Result (with no specific value).
     * @param success_tag Placeholder to indicate success.
     */
    Result(Success /*success_tag*/ = Success{}) : m_data(Success{}) {}

    /**
     * @brief Constructs an error Result.
     * @param error The error value.
     */
    Result(E error) : m_data(error) {}

    /**
     * @brief Checks if the result is a success.
     * @return True if the operation succeeded, false otherwise.
     */
    bool isSuccess() const {
        return std::holds_alternative<Success>(m_data);
    }

    /**
     * @brief Checks if the result is an error.
     * @return True if the operation failed, false otherwise.
     */
    bool isError() const {
        return std::holds_alternative<E>(m_data);
    }

    // No value() method for Result<void, E> as there's no value to return.

    /**
     * @brief Gets the error value.
     * @return The error value.
     * @throws std::runtime_error if called when not holding an error.
     */
    E error() const {
        if (!isError()) {
            throw std::runtime_error("Called error() on a success Result<void, E> or variant holds unexpected type.");
        }
        return std::get<E>(m_data);
    }
    
    // Operator bool for easy checking like if(result)
    explicit operator bool() const {
        return isSuccess();
    }

private:
    std::variant<Success, E> m_data;
};


/**
 * @brief Represents a bitmap image.
 */
struct Bitmap {
    uint32_t w;    ///< Width of the bitmap in pixels.
    uint32_t h;    ///< Height of the bitmap in pixels.
    uint32_t bpp;  ///< Bits per pixel (e.g., 24 for RGB, 32 for RGBA).
    std::vector<uint8_t> data; ///< Pixel data, typically in RGBA format.
};

/**
 * @brief Loads a bitmap from a memory span.
 *
 * Parses the BMP file data provided in the span and constructs a Bitmap object.
 *
 * @param bmp_data A span of constant uint8_t representing the raw BMP file data.
 * @return A Result object containing either a Bitmap on success or a BitmapError on failure.
 */
Result<Bitmap, BitmapError> load(std::span<const uint8_t> bmp_data);

/**
 * @brief Loads a bitmap from a file path.
 *
 * Opens and reads the BMP file from the given path, then constructs a Bitmap object.
 *
 * @param filepath The path to the BMP file.
 * @return A Result object containing either a Bitmap on success or a BitmapError on failure.
 */
Result<Bitmap, BitmapError> load(const std::string& filepath);

/**
 * @brief Saves a Bitmap object to a memory span as BMP file data.
 *
 * Converts the Bitmap object into the BMP file format and writes it to the provided output buffer.
 *
 * @param bitmap The Bitmap object to save.
 * @param out_bmp_buffer A span of uint8_t where the BMP file data will be written.
 *                       The span must be large enough to hold the entire BMP file.
 * @return A Result object containing Success on success or a BitmapError on failure.
 */
Result<void, BitmapError> save(const Bitmap& bitmap, std::span<uint8_t> out_bmp_buffer);

/**
 * @brief Saves a Bitmap object to a file path.
 *
 * Converts the Bitmap object into the BMP file format and writes it to the specified file.
 *
 * @param bitmap The Bitmap object to save.
 * @param filepath The path to the file where the BMP data will be saved.
 * @return A Result object containing Success on success or a BitmapError on failure.
 */
Result<void, BitmapError> save(const Bitmap& bitmap, const std::string& filepath);

/**
 * @brief Shrinks the image by a given scale factor.
 * @param bitmap The input bitmap.
 * @param scaleFactor The factor by which to shrink the image (e.g., 2 for half size). Must be positive.
 * @return Result containing the shrunken bitmap or an error.
 */
Result<Bitmap, BitmapError> shrink(const Bitmap& bitmap, int scaleFactor);

/**
 * @brief Rotates the image 90 degrees counter-clockwise.
 * @param bitmap The input bitmap.
 * @return Result containing the rotated bitmap or an error.
 */
Result<Bitmap, BitmapError> rotateCounterClockwise(const Bitmap& bitmap);

/**
 * @brief Rotates the image 90 degrees clockwise.
 * @param bitmap The input bitmap.
 * @return Result containing the rotated bitmap or an error.
 */
Result<Bitmap, BitmapError> rotateClockwise(const Bitmap& bitmap);

/**
 * @brief Mirrors the image horizontally.
 * @param bitmap The input bitmap.
 * @return Result containing the mirrored bitmap or an error.
 */
Result<Bitmap, BitmapError> mirror(const Bitmap& bitmap);

/**
 * @brief Flips the image vertically.
 * @param bitmap The input bitmap.
 * @return Result containing the flipped bitmap or an error.
 */
Result<Bitmap, BitmapError> flip(const Bitmap& bitmap);

/**
 * @brief Converts the image to greyscale.
 * @param bitmap The input bitmap.
 * @return Result containing the greyscale bitmap or an error.
 */
Result<Bitmap, BitmapError> greyscale(const Bitmap& bitmap);

/**
 * @brief Changes the overall brightness of the image.
 * @param bitmap The input bitmap.
 * @param brightness The brightness factor (e.g., 1.0 for no change, >1.0 for brighter, <1.0 for darker).
 * @return Result containing the bitmap with adjusted brightness or an error.
 */
Result<Bitmap, BitmapError> changeBrightness(const Bitmap& bitmap, float brightness);

/**
 * @brief Changes the overall contrast of the image.
 * @param bitmap The input bitmap.
 * @param contrast The contrast factor (e.g., 1.0 for no change).
 * @return Result containing the bitmap with adjusted contrast or an error.
 */
Result<Bitmap, BitmapError> changeContrast(const Bitmap& bitmap, float contrast);

/**
 * @brief Changes the overall saturation of the image.
 * @param bitmap The input bitmap.
 * @param saturation The saturation factor (e.g., 1.0 for no change, >1.0 for more saturated, <1.0 for less saturated).
 * @return Result containing the bitmap with adjusted saturation or an error.
 */
Result<Bitmap, BitmapError> changeSaturation(const Bitmap& bitmap, float saturation);

/**
 * @brief Changes the saturation of the blue channel in the image.
 * @param bitmap The input bitmap.
 * @param saturation The saturation factor for the blue channel.
 * @return Result containing the bitmap with adjusted blue channel saturation or an error.
 */
Result<Bitmap, BitmapError> changeSaturationBlue(const Bitmap& bitmap, float saturation);

/**
 * @brief Changes the saturation of the green channel in the image.
 * @param bitmap The input bitmap.
 * @param saturation The saturation factor for the green channel.
 * @return Result containing the bitmap with adjusted green channel saturation or an error.
 */
Result<Bitmap, BitmapError> changeSaturationGreen(const Bitmap& bitmap, float saturation);

/**
 * @brief Changes the saturation of the red channel in the image.
 * @param bitmap The input bitmap.
 * @param saturation The saturation factor for the red channel.
 * @return Result containing the bitmap with adjusted red channel saturation or an error.
 */
Result<Bitmap, BitmapError> changeSaturationRed(const Bitmap& bitmap, float saturation);

/**
 * @brief Changes the saturation of the magenta component (red and blue channels) in the image.
 * @param bitmap The input bitmap.
 * @param saturation The saturation factor for magenta.
 * @return Result containing the bitmap with adjusted magenta saturation or an error.
 */
Result<Bitmap, BitmapError> changeSaturationMagenta(const Bitmap& bitmap, float saturation);

/**
 * @brief Changes the saturation of the yellow component (red and green channels) in the image.
 * @param bitmap The input bitmap.
 * @param saturation The saturation factor for yellow.
 * @return Result containing the bitmap with adjusted yellow saturation or an error.
 */
Result<Bitmap, BitmapError> changeSaturationYellow(const Bitmap& bitmap, float saturation);

/**
 * @brief Changes the saturation of the cyan component (green and blue channels) in the image.
 * @param bitmap The input bitmap.
 * @param saturation The saturation factor for cyan.
 * @return Result containing the bitmap with adjusted cyan saturation or an error.
 */
Result<Bitmap, BitmapError> changeSaturationCyan(const Bitmap& bitmap, float saturation);

/**
 * @brief Changes the luminance of the blue channel in the image.
 * @param bitmap The input bitmap.
 * @param luminance The luminance factor for the blue channel.
 * @return Result containing the bitmap with adjusted blue channel luminance or an error.
 */
Result<Bitmap, BitmapError> changeLuminanceBlue(const Bitmap& bitmap, float luminance);

/**
 * @brief Changes the luminance of the green channel in the image.
 * @param bitmap The input bitmap.
 * @param luminance The luminance factor for the green channel.
 * @return Result containing the bitmap with adjusted green channel luminance or an error.
 */
Result<Bitmap, BitmapError> changeLuminanceGreen(const Bitmap& bitmap, float luminance);

/**
 * @brief Changes the luminance of the red channel in the image.
 * @param bitmap The input bitmap.
 * @param luminance The luminance factor for the red channel.
 * @return Result containing the bitmap with adjusted red channel luminance or an error.
 */
Result<Bitmap, BitmapError> changeLuminanceRed(const Bitmap& bitmap, float luminance);

/**
 * @brief Changes the luminance of the magenta component (red and blue channels) in the image.
 * @param bitmap The input bitmap.
 * @param luminance The luminance factor for magenta.
 * @return Result containing the bitmap with adjusted magenta luminance or an error.
 */
Result<Bitmap, BitmapError> changeLuminanceMagenta(const Bitmap& bitmap, float luminance);

/**
 * @brief Changes the luminance of the yellow component (red and green channels) in the image.
 * @param bitmap The input bitmap.
 * @param luminance The luminance factor for yellow.
 * @return Result containing the bitmap with adjusted yellow luminance or an error.
 */
Result<Bitmap, BitmapError> changeLuminanceYellow(const Bitmap& bitmap, float luminance);

/**
 * @brief Changes the luminance of the cyan component (green and blue channels) in the image.
 * @param bitmap The input bitmap.
 * @param luminance The luminance factor for cyan.
 * @return Result containing the bitmap with adjusted cyan luminance or an error.
 */
Result<Bitmap, BitmapError> changeLuminanceCyan(const Bitmap& bitmap, float luminance);

/**
 * @brief Inverts the colors of the image.
 * @param bitmap The input bitmap.
 * @return Result containing the inverted bitmap or an error.
 */
Result<Bitmap, BitmapError> invertColors(const Bitmap& bitmap);

/**
 * @brief Applies a sepia tone to the image.
 * @param bitmap The input bitmap.
 * @return Result containing the sepia toned bitmap or an error.
 */
Result<Bitmap, BitmapError> applySepiaTone(const Bitmap& bitmap);

/**
 * @brief Applies a box blur to the image.
 * @param bitmap The input bitmap.
 * @param blurRadius The radius of the blur box (e.g., 1 for a 3x3 box). Defaults to 1. Must be non-negative.
 * @return Result containing the blurred bitmap or an error.
 */
Result<Bitmap, BitmapError> applyBoxBlur(const Bitmap& bitmap, int blurRadius = 1);

/**
 * @brief Reduces the color bit-depth of image channels (Feature Squeezing).
 *
 * Quantizes 8-bit channel values to discrete 2^b levels and maps them back
 * to [0, 255] using uniform quantization:
 *   x' = round( round(x * (2^b - 1) / 255) * 255 / (2^b - 1) )
 *
 * This operation destroys low-amplitude adversarial micro-perturbations
 * (e.g., Fast Gradient Sign Method [FGSM], Projected Gradient Descent [PGD])
 * before passing images to neural network inference.
 *
 * @param bitmap The input bitmap (24bpp or 32bpp).
 * @param bitsPerChannel Target bit depth per channel, must be in [1, 8].
 * @param quantizeAlpha If true, also quantizes the alpha channel; defaults to false (preserving opacity).
 * @return Result containing the quantized bitmap or an error (e.g. InvalidColorDepth if bitsPerChannel not in [1, 8]).
 */
Result<Bitmap, BitmapError> quantizeChannels(const Bitmap& bitmap, uint8_t bitsPerChannel, bool quantizeAlpha = false);

/**
 * @brief Applies a non-linear median filter to the image for impulse and adversarial noise removal.
 *
 * Replaces each pixel's color channel with the median value of its neighboring pixels
 * within a square kernel (3x3 or 5x5). Eliminates single-pixel adversarial attacks,
 * salt-and-pepper noise, and extreme high-frequency spikes while preserving sharp edges.
 *
 * Edge boundaries are handled using border clamping (replicate edge pixels).
 *
 * @param bitmap The input bitmap (24bpp or 32bpp).
 * @param kernelSize The kernel dimension, must be 3 (3x3) or 5 (5x5). Defaults to 3.
 * @param preserveAlpha If true, preserves the alpha channel untouched (for 32bpp); defaults to true.
 * @return Result containing the filtered bitmap or an error (e.g. InvalidImageData if kernelSize != 3 and kernelSize != 5).
 */
Result<Bitmap, BitmapError> medianFilter(const Bitmap& bitmap, uint32_t kernelSize = 3, bool preserveAlpha = true);

/**
 * @brief Applies a 2D Gaussian blur filter to the image for spatial smoothing and noise reduction.
 *
 * Implements separable 1D horizontal and vertical convolution passes using a normalized
 * Gaussian kernel G(x) = exp(-x^2 / (2 * sigma^2)). Smooths out low-amplitude spatial noise
 * and adversarial gradient perturbations.
 *
 * Edge boundaries are handled using border clamping (replicate edge pixels).
 *
 * @param bitmap The input bitmap (24bpp or 32bpp).
 * @param sigma Standard deviation of the Gaussian distribution (must be positive and finite).
 * @param radius Kernel radius in pixels. If 0, auto-computed as ceil(3 * sigma). Clamped to MAX_SAFE_BLUR_RADIUS (64).
 * @param preserveAlpha If true, preserves the alpha channel untouched (for 32bpp); defaults to true.
 * @return Result containing the smoothed bitmap or an error.
 */
Result<Bitmap, BitmapError> applyGaussianBlur(const Bitmap& bitmap, float sigma, int32_t radius = 0, bool preserveAlpha = true);

/**
 * @brief Applies an edge-preserving bilateral filter to the image.
 *
 * Combines a geometric spatial Gaussian kernel with a photometric range Gaussian kernel:
 *   W(p, q) = exp(-||p - q||^2 / (2 * sigma_s^2)) * exp(-||I_p - I_q||^2 / (2 * sigma_r^2))
 * Smooths subtle textures and gradient noise within continuous surfaces while strictly
 * preserving high-contrast object contours and semantic edges.
 *
 * Edge boundaries are handled using border clamping (replicate edge pixels).
 *
 * @param bitmap The input bitmap (24bpp or 32bpp).
 * @param spatialSigma Spatial standard deviation (sigma_s, must be positive and finite).
 * @param rangeSigma Photometric range standard deviation (sigma_r, must be positive and finite).
 * @param radius Kernel radius in pixels. If 0, auto-computed as ceil(2 * spatialSigma). Clamped to [1, 16].
 * @param preserveAlpha If true, preserves the alpha channel untouched (for 32bpp); defaults to true.
 * @return Result containing the filtered bitmap or an error.
 */
Result<Bitmap, BitmapError> applyBilateralFilter(const Bitmap& bitmap, float spatialSigma, float rangeSigma, int32_t radius = 0, bool preserveAlpha = true);

/**
 * @brief Standard photometric weighting standards for RGB to grayscale luma conversion.
 */
enum class PhotometricStandard {
    BT601, ///< ITU-R BT.601 standard (SDTV): Y = 0.299*R + 0.587*G + 0.114*B
    BT709  ///< ITU-R BT.709 standard (sRGB / HDTV / modern vision models): Y = 0.2126*R + 0.7152*G + 0.0722*B
};

/**
 * @brief Converts an image to grayscale using physiologically and photometrically accurate luma weighting.
 *
 * Replaces RGB channels with the computed photometric luma value Y:
 *   BT.709: Y = 0.2126 * R + 0.7152 * G + 0.0722 * B
 *   BT.601: Y = 0.2990 * R + 0.5870 * G + 0.1140 * B
 *
 * Implemented using high-performance 16-bit fixed-point arithmetic for zero float latency.
 *
 * @param bitmap The input bitmap (24bpp or 32bpp).
 * @param standard Photometric standard to apply (defaults to BT709).
 * @param preserveAlpha If true, preserves the alpha channel untouched (for 32bpp); defaults to true.
 * @return Result containing the converted bitmap or an error.
 */
Result<Bitmap, BitmapError> extractPhotometricLuma(const Bitmap& bitmap, PhotometricStandard standard = PhotometricStandard::BT709, bool preserveAlpha = true);

/**
 * @brief Applies Local Contrast Normalization (LCN) to the image.
 *
 * Normalizes local image receptive fields by subtracting local context mean and dividing
 * by local standard deviation:
 *   I'(x, y) = clamp(128 + alpha * (I(x, y) - mu(x, y)) / (sigma(x, y) + epsilon), 0, 255)
 *
 * Neutralizes localized glare, lighting variations, and adversarial gradient brightness spikes.
 *
 * @param bitmap The input bitmap (24bpp or 32bpp).
 * @param sigma Neighborhood Gaussian blur radius standard deviation (defaults to 2.0f).
 * @param alpha Contrast scaling gain factor (defaults to 64.0f).
 * @param epsilon Variance stability threshold to avoid division by zero on flat areas (defaults to 1.0f).
 * @param preserveAlpha If true, preserves the alpha channel untouched (for 32bpp); defaults to true.
 * @return Result containing the normalized bitmap or an error.
 */
Result<Bitmap, BitmapError> localContrastNormalize(const Bitmap& bitmap, float sigma = 2.0f, float alpha = 64.0f, float epsilon = 1.0f, bool preserveAlpha = true);

} // namespace BmpTool



