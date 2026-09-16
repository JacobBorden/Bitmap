#pragma once

#include <cstdint>
#include <cstddef>
#include <limits>

namespace BmpTool::SafeMath {

constexpr uint32_t MAX_SAFE_DIMENSION = 65536;               // 64K max width/height
constexpr size_t MAX_SAFE_IMAGE_BYTES = 512 * 1024 * 1024;    // 512 MB memory limit
constexpr int MAX_SAFE_SCALE_FACTOR = 256;                   // Max scale factor for shrink/downscale
constexpr int MAX_SAFE_BLUR_RADIUS = 64;                     // Max kernel blur radius to prevent CPU DOS

template <typename T>
inline T clamp(T val, T minVal, T maxVal) {
    return (val < minVal) ? minVal : ((val > maxVal) ? maxVal : val);
}

/**
 * @brief Safely multiplies two uint32_t numbers, detecting overflow.
 */
inline bool multiply(uint32_t a, uint32_t b, uint32_t& out) {
    uint64_t prod = static_cast<uint64_t>(a) * static_cast<uint64_t>(b);
    if (prod > std::numeric_limits<uint32_t>::max()) {
        return false;
    }
    out = static_cast<uint32_t>(prod);
    return true;
}

/**
 * @brief Safely multiplies two size_t numbers, detecting overflow.
 */
inline bool multiply(size_t a, size_t b, size_t& out) {
    if (a != 0 && b > std::numeric_limits<size_t>::max() / a) {
        return false;
    }
    out = a * b;
    return true;
}

/**
 * @brief Safely adds two uint32_t numbers, detecting overflow.
 */
inline bool add(uint32_t a, uint32_t b, uint32_t& out) {
    if (std::numeric_limits<uint32_t>::max() - a < b) {
        return false;
    }
    out = a + b;
    return true;
}

/**
 * @brief Safely adds two size_t numbers, detecting overflow.
 */
inline bool add(size_t a, size_t b, size_t& out) {
    if (std::numeric_limits<size_t>::max() - a < b) {
        return false;
    }
    out = a + b;
    return true;
}

/**
 * @brief Safely extracts absolute image height, handling INT32_MIN and bounds.
 */
inline bool getSafeAbsoluteHeight(int32_t height, uint32_t& outAbsHeight) {
    if (height == 0 || height == std::numeric_limits<int32_t>::min()) {
        return false;
    }
    int64_t val = (height < 0) ? -static_cast<int64_t>(height) : static_cast<int64_t>(height);
    if (val > MAX_SAFE_DIMENSION) {
        return false;
    }
    outAbsHeight = static_cast<uint32_t>(val);
    return true;
}

/**
 * @brief Safely computes 4-byte aligned BMP row stride.
 */
inline bool computeRowStride(uint32_t width, uint16_t bpp, uint32_t& outStride) {
    if (width == 0 || width > MAX_SAFE_DIMENSION) {
        return false;
    }
    if (bpp != 24 && bpp != 32) {
        return false;
    }
    uint32_t bytesPerPixel = bpp / 8;
    uint32_t unpadded = 0;
    if (!multiply(width, bytesPerPixel, unpadded)) {
        return false;
    }
    uint32_t padded = 0;
    if (!add(unpadded, 3u, padded)) {
        return false;
    }
    outStride = padded & ~3u;
    return true;
}

/**
 * @brief Safely computes total expected pixel data size in bytes with sanity checks.
 */
inline bool computePixelDataSize(uint32_t width, uint32_t height, uint16_t bpp, uint32_t& outSize) {
    if (width == 0 || width > MAX_SAFE_DIMENSION || height == 0 || height > MAX_SAFE_DIMENSION) {
        return false;
    }
    uint32_t stride = 0;
    if (!computeRowStride(width, bpp, stride)) {
        return false;
    }
    if (!multiply(stride, height, outSize)) {
        return false;
    }
    if (outSize > MAX_SAFE_IMAGE_BYTES) {
        return false;
    }
    return true;
}

} // namespace BmpTool::SafeMath
