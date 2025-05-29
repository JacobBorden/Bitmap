#include "../../src/matrix/matrix.h" // For Matrix::Matrix
#include <cstdint>
#include <vector>
// #include <string> // Not strictly needed for this fuzzer's core logic
#include <cstring>   // For std::memcpy
#include <algorithm> // For std::min, std::max
#include <stdexcept> // For std::bad_alloc, std::out_of_range, std::invalid_argument, std::runtime_error
#include <limits>    // For std::numeric_limits
#include <cmath>     // For std::isnan, std::isinf

// Helper to consume data from the fuzzer input
template <typename T>
T Consume(const uint8_t** data_ptr, size_t* size_ptr) {
    if (*size_ptr < sizeof(T)) {
        return T{};
    }
    T value;
    std::memcpy(&value, *data_ptr, sizeof(T));
    *data_ptr += sizeof(T);
    *size_ptr -= sizeof(T);
    return value;
}

// Helper to consume a specific type, e.g., double
template <typename T>
T ConsumeValue(const uint8_t** data_ptr, size_t* size_ptr) {
    if (*size_ptr < sizeof(T)) {
        if (std::is_floating_point<T>::value) {
            if (*size_ptr > 0 && *data_ptr != nullptr) { 
                T temp_val = 0;
                unsigned char* p_temp_val = reinterpret_cast<unsigned char*>(&temp_val);
                for(size_t i=0; i < std::min(sizeof(T), *size_ptr); ++i) {
                    p_temp_val[i] = (*data_ptr)[i];
                }
                 if (std::isnan(temp_val) || std::isinf(temp_val)) return static_cast<T>(1.0);
                 return temp_val;
            }
            return static_cast<T>(1.0); 
        }
        return T{}; 
    }
    T val;
    std::memcpy(&val, *data_ptr, sizeof(T));
    *data_ptr += sizeof(T);
    *size_ptr -= sizeof(T);

    if (std::is_floating_point<T>::value) {
        if (std::isnan(val) || std::isinf(val)) {
            // Replace NaN/inf with a deterministic value based on its first byte if possible
            if (sizeof(T) > 0) return static_cast<T>( (reinterpret_cast<const uint8_t*>(&val)[0] % 100) + 0.5);
            return static_cast<T>(1.0); // Fallback if sizeof(T) is 0 (should not happen)
        }
    }
    return val;
}


using MatrixType = double;
// Reduced MAX_DIM to keep memory/time per run lower for fuzzing many ops. (e.g. 32x32)
const int MAX_DIM = 32; 

Matrix::Matrix<MatrixType> CreateFuzzedMatrix(const uint8_t** Data, size_t* Size) {
    uint8_t rows_byte = Consume<uint8_t>(Data, Size);
    uint8_t cols_byte = Consume<uint8_t>(Data, Size);

    int r = rows_byte % MAX_DIM;
    int c = cols_byte % MAX_DIM;
    
    uint8_t non_zero_choice = Consume<uint8_t>(Data, Size); // To decide if dimensions should be forced non-zero
    if (non_zero_choice % 10 != 0) { // ~90% of time, try to make dimensions non-zero if they landed on 0
        if (r == 0) r = 1 + (rows_byte % std::max(1, MAX_DIM-1)); // Ensure at least 1 if was 0
        if (c == 0) c = 1 + (cols_byte % std::max(1, MAX_DIM-1)); // Ensure at least 1 if was 0
    } else { // ~10% of time, allow actual 0 if byte % MAX_DIM was 0
         // If original byte was >= MAX_DIM (so it wrapped to 0), but we are in the "allow zero" path,
         // it means we want it to be zero. But if it was, say, 1%MAX_DIM == 1, it stays 1.
         // This logic is okay, r and c are already set.
    }
    if (r < 0) r = 0; // Should be impossible due to % MAX_DIM (non-negative)
    if (c < 0) c = 0;


    try {
        Matrix::Matrix<MatrixType> m(r, c);
        for (int i = 0; i < r; ++i) {
            for (int j = 0; j < c; ++j) {
                // Allow consuming partial data for last element, or use default.
                if (*Size >= sizeof(MatrixType) / 2 && *Size > 0 ) { 
                    m[i][j] = ConsumeValue<MatrixType>(Data, Size);
                } else {
                    // Default pattern if not enough data for a full MatrixType or if Size is too small
                    m[i][j] = static_cast<MatrixType>( (i+j) % 3 + 1 ); // e.g. 1,2,3,1,2,3...
                }
            }
        }
        return m;
    } catch (const std::bad_alloc&) {
        return Matrix::Matrix<MatrixType>(0,0); // Return empty on allocation failure
    } catch (const std::out_of_range&) {
         // This might happen if MatrixRow constructor or resize fails in a specific way
         return Matrix::Matrix<MatrixType>(0,0); 
    }
}


extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    if (Size < 5) { 
        return 0;
    }

    uint8_t operation_choice = Consume<uint8_t>(&Data, &Size);

    Matrix::Matrix<MatrixType> m1 = CreateFuzzedMatrix(&Data, &Size);
    Matrix::Matrix<MatrixType> m2; 
    
    // Conditionally create m2 only for operations that need it
    bool m2_needed = false;
    int op_mod = operation_choice % 23; // Number of cases
    if (op_mod == 3 || op_mod == 4 || op_mod == 5 || op_mod == 10 || op_mod == 11 || op_mod == 18 || op_mod == 19) {
        m2_needed = true;
    }

    if (m2_needed) {
        if (Size > 2) { // Min data for m2 dimensions (2 bytes for rows/cols byte)
             m2 = CreateFuzzedMatrix(&Data, &Size);
        } else { 
            // Not enough data for a full m2, create a default one that might match m1 for some ops
            uint8_t choice = Consume<uint8_t>(&Data, &Size); // Consume last byte if any
            try {
                if (choice % 2 == 0 && m1.rows() > 0 && m1.cols() > 0 && m1.rows() < MAX_DIM && m1.cols() < MAX_DIM) {
                     m2.resize(m1.rows(), m1.cols()); 
                } else {
                     m2.resize(choice % MAX_DIM, (Size > 0 ? Consume<uint8_t>(&Data, &Size) : choice) % MAX_DIM);
                }
                 // Populate m2 with default values if resized
                for(size_t r_idx = 0; r_idx < m2.rows(); ++r_idx) {
                    for(size_t c_idx = 0; c_idx < m2.cols(); ++c_idx) {
                        m2[r_idx][c_idx] = static_cast<MatrixType>((r_idx + c_idx) % 2 + 1);
                    }
                }
            } catch(...) { /* Ignore errors creating this fallback m2 */ }
        }
    }


    Matrix::Matrix<MatrixType> result_matrix;
    MatrixType result_scalar = 0;
    (void)result_scalar; 

    try {
        switch (op_mod) { 
            case 0: if (!m1.empty()) result_matrix = m1.Transpose(); break;
            case 1:
                if (m1.rows() == m1.cols() && !m1.empty()) {
                    result_scalar = m1.Determinant();
                }
                break;
            case 2:
                if (m1.rows() == m1.cols() && !m1.empty()) {
                    result_matrix = m1.Inverse(); 
                }
                break;
            case 3: result_matrix = m1 + m2; break;
            case 4: result_matrix = m1 - m2; break;
            case 5: result_matrix = m1 * m2; break; 
            case 6: result_matrix = m1 * ConsumeValue<MatrixType>(&Data, &Size); break; 
            case 7: result_matrix = m1 + ConsumeValue<MatrixType>(&Data, &Size); break; 
            case 8: result_matrix = m1 - ConsumeValue<MatrixType>(&Data, &Size); break; 
            case 9: 
                {
                    MatrixType scalar = ConsumeValue<MatrixType>(&Data, &Size);
                    if (std::abs(scalar) < std::numeric_limits<MatrixType>::epsilon() * 100 && scalar != 0) { /* scalar is tiny */ }
                    else if (scalar == 0) scalar = 1.0; // Avoid division by zero explicitly
                    result_matrix = m1 / scalar; 
                }
                break;
            case 10: m1 += m2; break; 
            case 11: m1 -= m2; break;
            case 12: m1 *= ConsumeValue<MatrixType>(&Data, &Size); break;
            case 13: 
                {
                    MatrixType scalar = ConsumeValue<MatrixType>(&Data, &Size);
                     if (std::abs(scalar) < std::numeric_limits<MatrixType>::epsilon() * 100 && scalar != 0) { /* scalar is tiny */ }
                    else if (scalar == 0) scalar = 1.0; // Avoid division by zero
                    m1 /= scalar; 
                }
                break;
            case 14: if (!m1.empty()) m1.ZeroMatrix(); break;
            case 15: if (m1.rows() == m1.cols() && !m1.empty()) m1.CreateIdentityMatrix(); break; 
            case 16: if (!m1.empty()) m1.Randomize(); break;
            case 17: if (!m1.empty()) m1.SigmoidMatrix(); break;
            case 18: result_matrix = m1.MergeHorizontal(m2); break;
            case 19: result_matrix = m1.MergeVertical(m2); break;
            case 20: 
                 {
                    uint8_t r_byte = Consume<uint8_t>(&Data, &Size);
                    uint8_t c_byte = Consume<uint8_t>(&Data, &Size);
                    m1.resize(r_byte % MAX_DIM, c_byte % MAX_DIM);
                 }
                break;
            case 21: 
                 {
                    uint8_t r_byte = Consume<uint8_t>(&Data, &Size);
                    uint8_t c_byte = Consume<uint8_t>(&Data, &Size);
                    MatrixType val = ConsumeValue<MatrixType>(&Data, &Size);
                    m1.assign(r_byte % MAX_DIM, c_byte % MAX_DIM, val);
                 }
                break;
             case 22: 
                if (m1.rows() > 0 && m1.cols() > 0) {
                    size_t r_idx = Consume<uint8_t>(&Data, &Size) % m1.rows();
                    size_t c_idx = Consume<uint8_t>(&Data, &Size) % m1.cols();
                    [[maybe_unused]] MatrixType val_at = m1.at(r_idx, c_idx);
                }
                break;
            // Default case removed to ensure all op_mod values map to a defined case.
        }
    } catch (const std::bad_alloc&) { 
    } catch (const std::out_of_range&) { 
    } catch (const std::invalid_argument&) { 
    } catch (const std::runtime_error&) { 
    } 

    return 0;
}
