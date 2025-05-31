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
    UnknownError          ///< An unspecified error occurred.
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

} // namespace BmpTool
