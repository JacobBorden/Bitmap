#pragma once // Use pragma once for include guard

#include <cstdint> // For uint32_t, uint8_t
#include <vector>   // For std::vector
#include <variant>  // For std::variant in Result
#include <string>   // For error messages if needed (though enum is primary)
#include <span>     // For std::span (will be used by load/save)
#include <stdexcept> // For std::runtime_error in Result::value()

namespace BmpTool {

/**
 * @brief Represents errors that can occur during Bitmap operations.
 */
enum class BitmapError {
    Ok,                   ///< Operation completed successfully.
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
     * @throw std::runtime_error if the Result holds an error.
     */
    T value() const {
        if (isSuccess()) {
            return std::get<T>(m_data);
        }
        throw std::runtime_error("Attempted to access value from an error Result.");
    }

    /**
     * @brief Gets the error.
     * @return The stored error.
     * @throw std::runtime_error if the Result holds a success value.
     */
    E error() const {
        if (isError()) {
            return std::get<E>(m_data);
        }
        throw std::runtime_error("Attempted to access error from a success Result.");
    }

private:
    std::variant<T, E> m_data;
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
 * @return A Result object containing void on success (indicated by BitmapError::Ok)
 *         or a BitmapError on failure.
 */
Result<void, BitmapError> save(const Bitmap& bitmap, std::span<uint8_t> out_bmp_buffer);

} // namespace BmpTool
