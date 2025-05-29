#pragma once

#ifndef MATRIX_H
#define MATRIX_H

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <iterator>
#include <vector>
#include <cmath>
#include <random>
#include <chrono> // For std::chrono::system_clock
#include <utility> // For std::move

namespace Matrix
{
	template <typename MatrixRowType> // Changed template parameter name to avoid conflict
	class MatrixRowIterator
	{
	public:
		using value_type = typename MatrixRowType::value_type;
		using pointer = value_type *;
		using reference = value_type &;
		using iterator_category = std::random_access_iterator_tag;
		using difference_type = std::ptrdiff_t;

		MatrixRowIterator(pointer ptr) : m_ptr(ptr) {}

		MatrixRowIterator &operator++() { m_ptr++; return *this; }
		MatrixRowIterator operator++(int) { MatrixRowIterator it = *this; ++*this; return it; }
		MatrixRowIterator operator+(difference_type n) const { return MatrixRowIterator(m_ptr + n); }
		MatrixRowIterator &operator+=(difference_type n) { m_ptr += n; return *this; }
		MatrixRowIterator &operator--() { m_ptr--; return *this; }
		MatrixRowIterator operator--(int) { MatrixRowIterator it = *this; --*this; return it; }
		MatrixRowIterator operator-(difference_type n) const { return MatrixRowIterator(m_ptr - n); }
		MatrixRowIterator &operator-=(difference_type n) { m_ptr -= n; return *this; }
		difference_type operator-(const MatrixRowIterator &other) const { return m_ptr - other.m_ptr; }
		pointer operator->() const { return m_ptr; }
		reference operator*() { return *m_ptr; }
		const reference operator*() const { return *m_ptr; }
		bool operator==(const MatrixRowIterator &other) const { return m_ptr == other.m_ptr; }
		bool operator!=(const MatrixRowIterator &other) const { return m_ptr != other.m_ptr; }
		bool operator<(const MatrixRowIterator &other) const { return m_ptr < other.m_ptr; }
		bool operator<=(const MatrixRowIterator &other) const { return m_ptr <= other.m_ptr; }
		bool operator>(const MatrixRowIterator &other) const { return m_ptr > other.m_ptr; }
		bool operator>=(const MatrixRowIterator &other) const { return m_ptr >= other.m_ptr; }
		reference operator[](difference_type n) const { return *(*this + n); }

	private:
		pointer m_ptr;
	};

	template <typename T>
	class MatrixColumnIterator
	{
	public:
		using value_type = T;
		using pointer = T *;
		using reference = T &;
		using iterator_category = std::random_access_iterator_tag;
		using difference_type = std::ptrdiff_t;

		MatrixColumnIterator(pointer ptr, size_t totalColumns) : m_ptr(ptr), m_totalColumns(totalColumns) {}

		MatrixColumnIterator &operator++() { m_ptr += m_totalColumns; return *this; }
		MatrixColumnIterator operator++(int) { MatrixColumnIterator it = *this; m_ptr += m_totalColumns; return it; }
		MatrixColumnIterator operator+(difference_type n) const { return MatrixColumnIterator(m_ptr + (n * m_totalColumns), m_totalColumns); }
		MatrixColumnIterator &operator+=(difference_type n) { m_ptr += (n * m_totalColumns); return *this; }
		MatrixColumnIterator &operator--() { m_ptr -= m_totalColumns; return *this; }
		MatrixColumnIterator operator--(int) { MatrixColumnIterator it = *this; m_ptr -= m_totalColumns; return it; }
		MatrixColumnIterator operator-(difference_type n) const { return MatrixColumnIterator(m_ptr - (n * m_totalColumns), m_totalColumns); }
		MatrixColumnIterator &operator-=(difference_type n) { m_ptr -= (n * m_totalColumns); return *this; }
		difference_type operator-(const MatrixColumnIterator &other) const { return (m_ptr - other.m_ptr) / m_totalColumns; }
		bool operator==(const MatrixColumnIterator &other) const { return m_ptr == other.m_ptr; }
		bool operator!=(const MatrixColumnIterator &other) const { return m_ptr != other.m_ptr; }
		bool operator<(const MatrixColumnIterator &other) const { return m_ptr < other.m_ptr; }
		bool operator<=(const MatrixColumnIterator &other) const { return m_ptr <= other.m_ptr; }
		bool operator>(const MatrixColumnIterator &other) const { return m_ptr > other.m_ptr; }
		bool operator>=(const MatrixColumnIterator &other) const { return m_ptr >= other.m_ptr; }
		reference operator*() const { return *m_ptr; }
		pointer operator->() const { return m_ptr; }
		reference operator[](difference_type n) const { return *(*this + n); }

	private:
		pointer m_ptr;
		size_t m_totalColumns;
	};

	template <typename MatrixType> // Changed template parameter name
	class MatrixIterator
	{
	public:
		using value_type = typename MatrixType::value_type;
		using pointer = value_type *;
		using reference = value_type &;

		MatrixIterator(pointer ptr) : m_ptr(ptr) {}

		MatrixIterator &operator++() { m_ptr++; return *this; }
		MatrixIterator operator++(int) { MatrixIterator it = *this; ++(*this); return it; } // Corrected post-increment
		MatrixIterator &operator--() { m_ptr--; return *this; }
		MatrixIterator operator--(int) { MatrixIterator it = *this; --(*this); return it; } // Corrected post-decrement
		pointer operator->() { return m_ptr; }
		reference operator*() { return *m_ptr; }
		bool operator==(const MatrixIterator& other) const { return this->m_ptr == other.m_ptr; } // Added const and pass by ref
		bool operator!=(const MatrixIterator& other) const { return this->m_ptr != other.m_ptr; } // Added const and pass by ref

	private:
		pointer m_ptr;
	};

	template <typename T>
	class MatrixRow
	{
	public:
		using value_type = T;
		using Iterator = MatrixRowIterator<MatrixRow<T>>; // Corrected template argument

		MatrixRow() : m_Size(0), m_Capacity(0), m_Data(nullptr) {} // Default constructor

		explicit MatrixRow(size_t size) : m_Size(size), m_Capacity(size), m_Data(size > 0 ? std::make_unique<T[]>(size) : nullptr) {
            if (size > 0) {
                std::fill_n(m_Data.get(), m_Size, T{}); // Default initialize elements
            }
        }

		// Copy constructor
		MatrixRow(const MatrixRow& other) : m_Size(other.m_Size), m_Capacity(other.m_Capacity), m_Data(other.m_Size > 0 ? std::make_unique<T[]>(other.m_Size) : nullptr) {
			if (m_Size > 0) {
				std::copy_n(other.m_Data.get(), m_Size, m_Data.get());
			}
		}

		// Copy assignment operator
		MatrixRow& operator=(const MatrixRow& other) {
			if (this == &other) {
				return *this;
			}
			m_Size = other.m_Size;
			m_Capacity = other.m_Capacity;
			m_Data = (other.m_Size > 0 ? std::make_unique<T[]>(other.m_Size) : nullptr);
			if (m_Size > 0) {
				std::copy_n(other.m_Data.get(), m_Size, m_Data.get());
			}
			return *this;
		}

		// Move constructor
		MatrixRow(MatrixRow&& other) noexcept : m_Size(other.m_Size), m_Capacity(other.m_Capacity), m_Data(std::move(other.m_Data)) {
			other.m_Size = 0;
			other.m_Capacity = 0;
		}

		// Move assignment operator
		MatrixRow& operator=(MatrixRow&& other) noexcept {
			if (this == &other) {
				return *this;
			}
			m_Size = other.m_Size;
			m_Capacity = other.m_Capacity;
			m_Data = std::move(other.m_Data);
			other.m_Size = 0;
			other.m_Capacity = 0;
			return *this;
		}
        
        ~MatrixRow() = default; // Default destructor is fine with unique_ptr

		void resize(size_t newSize) {
			auto newData = (newSize > 0 ? std::make_unique<T[]>(newSize) : nullptr);
            if (m_Data) { // Only copy if old data exists
			    std::copy_n(m_Data.get(), std::min(m_Size, newSize), newData.get());
            }
			m_Data = std::move(newData);
			m_Size = newSize;
			m_Capacity = newSize; // Capacity is same as size for simplicity here
            if (newSize > m_Size && m_Data) { // If grown, default initialize new elements
                 std::fill_n(m_Data.get() + m_Size, newSize - m_Size, T{});
            }
		}

		void assign(size_t size, T val) {
			resize(size);
            if (size > 0) {
			    std::fill_n(m_Data.get(), size, val);
            }
		}

		void assign(T val) { 
            if (m_Size > 0) {
                std::fill_n(m_Data.get(), m_Size, val); 
            }
        }

		size_t size() const { return m_Size; }
		size_t capacity() const { return m_Capacity; } // Corrected to return m_Capacity

		T at(size_t i) const { // Added at()
			if (i >= m_Size)
				throw std::out_of_range("Index out of range");
			return m_Data[i];
		}
        T& at(size_t i) { // Added non-const at()
			if (i >= m_Size)
				throw std::out_of_range("Index out of range");
			return m_Data[i];
		}


		T &operator[](size_t i) {
			// No bounds check for performance, similar to std::vector
			return m_Data[i];
		}

		const T &operator[](size_t i) const {
			// No bounds check for performance
			return m_Data[i];
		}

		Iterator begin() { return Iterator(m_Data.get()); }
		Iterator end() { return Iterator(m_Data.get() + m_Size); }
		Iterator begin() const { return Iterator(m_Data.get()); } // Const version
		Iterator end() const { return Iterator(m_Data.get() + m_Size); } // Const version


	private:
		size_t m_Size = 0;
		size_t m_Capacity = 0;
		std::unique_ptr<T[]> m_Data;
	};


	template <typename T>
	class Matrix
	{
	public:
		using value_type = MatrixRow<T>;
		using Iterator = MatrixIterator<Matrix<T>>; // Corrected template argument
		using ColumnIterator = MatrixColumnIterator<T>; // Corrected, was ColumonIterator and wrong type

		Matrix() = default;
		explicit Matrix(size_t row_count, size_t column_count)
			: m_Rows(row_count), m_Cols(column_count), m_Size(row_count * column_count), m_Capacity(row_count), m_Data(row_count > 0 ? std::make_unique<MatrixRow<T>[]>(row_count) : nullptr)
		{
			if (row_count > 0) {
				for (size_t i = 0; i < m_Rows; i++)
					m_Data[i] = MatrixRow<T>(m_Cols); // Each row initialized with default T values
			}
		}
        // Constructor with initial value
        Matrix(size_t row_count, size_t column_count, const T& initial_value)
            : m_Rows(row_count), m_Cols(column_count), m_Size(row_count * column_count), m_Capacity(row_count), m_Data(row_count > 0 ? std::make_unique<MatrixRow<T>[]>(row_count) : nullptr)
        {
            if (row_count > 0) {
                for (size_t i = 0; i < m_Rows; i++) {
                    m_Data[i].assign(m_Cols, initial_value);
                }
            }
        }


		// Copy constructor
		Matrix(const Matrix& other) : m_Rows(other.m_Rows), m_Cols(other.m_Cols), m_Size(other.m_Size), m_Capacity(other.m_Capacity), m_Data(other.m_Rows > 0 ? std::make_unique<MatrixRow<T>[]>(other.m_Rows) : nullptr) {
			if (m_Rows > 0) {
				for (size_t i = 0; i < m_Rows; ++i) {
					m_Data[i] = other.m_Data[i]; // Uses MatrixRow's copy assignment
				}
			}
		}

		// Copy assignment operator
		Matrix& operator=(const Matrix& other) {
			if (this == &other) {
				return *this;
			}
			m_Rows = other.m_Rows;
			m_Cols = other.m_Cols;
			m_Size = other.m_Size;
			m_Capacity = other.m_Capacity;
			m_Data = (other.m_Rows > 0 ? std::make_unique<MatrixRow<T>[]>(other.m_Rows) : nullptr);
            if (m_Rows > 0) {
			    for (size_t i = 0; i < m_Rows; ++i) {
				    m_Data[i] = other.m_Data[i]; // Uses MatrixRow's copy assignment
			    }
            }
			return *this;
		}

		// Move constructor
		Matrix(Matrix&& other) noexcept 
            : m_Rows(other.m_Rows), m_Cols(other.m_Cols), m_Size(other.m_Size), m_Capacity(other.m_Capacity), m_Data(std::move(other.m_Data)) {
			other.m_Rows = 0;
			other.m_Cols = 0;
			other.m_Size = 0;
            other.m_Capacity = 0;
		}

		// Move assignment operator
		Matrix& operator=(Matrix&& other) noexcept {
			if (this == &other) {
				return *this;
			}
			m_Rows = other.m_Rows;
			m_Cols = other.m_Cols;
			m_Size = other.m_Size;
            m_Capacity = other.m_Capacity;
			m_Data = std::move(other.m_Data);
			other.m_Rows = 0;
			other.m_Cols = 0;
			other.m_Size = 0;
            other.m_Capacity = 0;
			return *this;
		}
        
        ~Matrix() = default; // Default destructor fine with unique_ptr

		size_t size() const { return m_Size; }
		size_t rows() const { return m_Rows; }
		size_t cols() const { return m_Cols; }
		size_t capacity() const { return m_Capacity; } // This capacity is for number of rows unique_ptr can hold.
        bool empty() const { return m_Rows == 0 || m_Cols == 0; } // Added empty()

		void resize(size_t row_count, size_t col_count) {
            auto newData = (row_count > 0 ? std::make_unique<MatrixRow<T>[]>(row_count) : nullptr);
            if (m_Data) { // If old data exists
			    for (size_t i = 0; i < std::min(m_Rows, row_count); ++i) {
				    newData[i] = std::move(m_Data[i]); // Move existing rows
			    }
            }
            // For new rows (if any), or if old data didn't exist
            for (size_t i = (m_Data ? std::min(m_Rows, row_count) : 0); i < row_count; ++i) {
                 newData[i] = MatrixRow<T>(col_count); // Initialize new rows
            }

            // Resize all rows to new column count
            for (size_t i = 0; i < row_count; ++i) {
                 newData[i].resize(col_count);
            }

			m_Data = std::move(newData);
			m_Rows = row_count;
			m_Cols = col_count;
			m_Size = row_count * col_count;
			m_Capacity = row_count; 
		}
        // resize with value
        void resize(size_t row_count, size_t col_count, const T& val) {
            Matrix<T> temp(row_count, col_count, val); // Create a temp matrix with the value
            if (m_Data) { // Preserve old data that fits
                for (size_t i = 0; i < std::min(m_Rows, row_count); ++i) {
                    for (size_t j = 0; j < std::min(m_Cols, col_count); ++j) {
                        temp[i][j] = m_Data[i][j];
                    }
                }
            }
            *this = std::move(temp); // Move assign
        }


		void assign(size_t row_count, size_t col_count, const T& val) {
			resize(row_count, col_count); // Resize first (might create default T values)
			for (size_t i = 0; i < m_Rows; ++i) { // Then assign specific value
				m_Data[i].assign(m_Cols, val);
            }
		}

		void assign(const T& val) { // Corrected: const T& val
			for (size_t i = 0; i < m_Rows; ++i)
				for (size_t j = 0; j < m_Cols; ++j)
					m_Data[i][j] = val;
		}
        
        // at() methods
        T at(size_t r, size_t c) const {
            if (r >= m_Rows || c >= m_Cols) throw std::out_of_range("Matrix index out of range");
            return m_Data[r][c];
        }
        T& at(size_t r, size_t c) {
            if (r >= m_Rows || c >= m_Cols) throw std::out_of_range("Matrix index out of range");
            return m_Data[r][c];
        }


		Matrix<T> MergeVertical(const Matrix<T> &b) const {
			if (m_Cols != b.m_Cols && !empty() && !b.empty()) // Allow merging with empty if one is empty
				throw std::invalid_argument("Matrices must have the same number of columns to merge vertically (unless one is empty)");
			
            size_t result_cols = empty() ? b.m_Cols : m_Cols;
            if (result_cols == 0 && !b.empty()) result_cols = b.m_Cols; // Handle case where this is empty but b is not

			Matrix<T> result(m_Rows + b.m_Rows, result_cols);
			for(size_t i=0; i<m_Rows; ++i) result[i] = m_Data[i]; // MatrixRow copy assignment
			for(size_t i=0; i<b.m_Rows; ++i) result[i + m_Rows] = b.m_Data[i];
			return result;
		}

		Matrix<T> MergeHorizontal(const Matrix<T> &b) const {
			if (m_Rows != b.m_Rows && !empty() && !b.empty())
				throw std::invalid_argument("Matrices must have the same number of rows to merge horizontally (unless one is empty)");
            
            size_t result_rows = empty() ? b.m_Rows : m_Rows;
             if (result_rows == 0 && !b.empty()) result_rows = b.m_Rows;


			Matrix<T> result(result_rows, m_Cols + b.m_Cols);
			for (size_t i = 0; i < result_rows; ++i) {
                if (i < m_Rows) { // If this matrix contributes the row
				    std::copy_n(m_Data[i].begin(), m_Cols, result.m_Data[i].begin());
                }
                if (i < b.m_Rows) { // If b matrix contributes the row
				    std::copy_n(b.m_Data[i].begin(), b.m_Cols, result.m_Data[i].begin() + m_Cols);
                }
			}
			return result;
		}

		std::vector<Matrix<T>> SplitVertical(size_t num_splits) const { // Renamed num to num_splits for clarity
            if (num_splits == 0) throw std::invalid_argument("Number of splits cannot be zero.");
            if (empty()) throw std::invalid_argument("Cannot split an empty matrix.");
			if (m_Rows % num_splits != 0)
				throw std::invalid_argument("Number of splits must evenly divide the number of rows");
			
            std::vector<Matrix<T>> result;
            result.reserve(num_splits);
			size_t split_size = m_Rows / num_splits;
			for (size_t i = 0; i < num_splits; ++i) {
				Matrix<T> split(split_size, m_Cols);
				for(size_t k=0; k < split_size; ++k) {
                    split[k] = m_Data[i * split_size + k]; // MatrixRow copy assignment
                }
				result.push_back(std::move(split)); // Use move
			}
			return result;
		}
        // Overload for splitting in half
        std::vector<Matrix<T>> SplitVertical() const { return SplitVertical(2); }


		std::vector<Matrix<T>> SplitHorizontal(size_t num_splits) const {
            if (num_splits == 0) throw std::invalid_argument("Number of splits cannot be zero.");
            if (empty()) throw std::invalid_argument("Cannot split an empty matrix.");
			if (m_Cols % num_splits != 0)
				throw std::invalid_argument("Number of splits must evenly divide the number of columns");

			std::vector<Matrix<T>> result;
            result.reserve(num_splits);
			size_t split_size = m_Cols / num_splits;
			for (size_t i = 0; i < num_splits; ++i) {
				Matrix<T> split(m_Rows, split_size);
				for (size_t j = 0; j < m_Rows; ++j) {
                    // Copy elements for the current horizontal segment of row j
                    for(size_t k=0; k < split_size; ++k) {
                        split[j][k] = m_Data[j][i * split_size + k];
                    }
				}
				result.push_back(std::move(split));
			}
			return result;
		}
        std::vector<Matrix<T>> SplitHorizontal() const { return SplitHorizontal(2); }


		Matrix<T>& SigmoidMatrix() { // Return by reference, not const
			for (size_t i = 0; i < m_Rows; ++i) {
				for (size_t j = 0; j < m_Cols; ++j) {
					m_Data[i][j] = T(1) / (T(1) + std::exp(-m_Data[i][j]));
				}
			}
            return *this;
		}

		Matrix<T>& Randomize() { // Return by reference
			static std::mt19937 gen(std::chrono::system_clock::now().time_since_epoch().count());
			std::uniform_real_distribution<double> dis(-1.0, 1.0); // Use double for distribution
			for (size_t i = 0; i < m_Rows; ++i) {
				for (size_t j = 0; j < m_Cols; ++j) {
					m_Data[i][j] = static_cast<T>(dis(gen));
				}
			}
			return *this;
		}
		Matrix<T>& CreateIdentityMatrix() { // Return by reference
			if (m_Rows != m_Cols)
				throw std::invalid_argument("Matrix must be square to become an identity matrix.");
            if (m_Rows == 0) return *this; // Or throw, but identity of 0x0 is tricky.
			for (size_t i = 0; i < m_Rows; ++i) {
				m_Data[i].assign(m_Cols, T(0)); // Zero out row first
				m_Data[i][i] = T(1);
			}
			return *this;
		}

		Matrix<T>& ZeroMatrix() { // Not const, return by reference
			for (size_t i = 0; i < m_Rows; ++i) {
				m_Data[i].assign(m_Cols, T(0));
			}
			return *this;
		}

		Matrix<T> Transpose() const {
            if (empty()) return Matrix<T>(); // Transpose of empty is empty
			Matrix<T> result(m_Cols, m_Rows);
			for (size_t i = 0; i < m_Rows; ++i)
				for (size_t j = 0; j < m_Cols; ++j)
					result[j][i] = m_Data[i][j];
			return result;
		}

		T Determinant() const {
			if (m_Rows != m_Cols)
				throw std::invalid_argument("Matrix must be square to calculate determinant.");
            if (m_Rows == 0) return T(1); // Determinant of 0x0 matrix is 1 by convention
			size_t n = m_Rows;
			if (n == 1)
				return m_Data[0][0];
			else if (n == 2)
				return m_Data[0][0] * m_Data[1][1] - m_Data[0][1] * m_Data[1][0];
			
            T det = T(0);
            Matrix<T> temp_matrix = *this; // Make a mutable copy for LU decomposition approach (more stable)
            
            for (size_t i = 0; i < n; ++i) {
                // Partial pivoting: find row with max element in current column
                size_t max_row = i;
                for (size_t k = i + 1; k < n; ++k) {
                    if (std::abs(temp_matrix[k][i]) > std::abs(temp_matrix[max_row][i])) {
                        max_row = k;
                    }
                }
                if (i != max_row) {
                    std::swap(temp_matrix.m_Data[i], temp_matrix.m_Data[max_row]);
                    // det sign changes with row swap, but this is handled by product of diagonal later
                }

                if (temp_matrix[i][i] == T(0)) return T(0); // Singular if pivot is zero

                for (size_t k = i + 1; k < n; ++k) {
                    T factor = temp_matrix[k][i] / temp_matrix[i][i];
                    for (size_t j = i; j < n; ++j) {
                        temp_matrix[k][j] -= factor * temp_matrix[i][j];
                    }
                }
            }
            // Determinant is the product of diagonal elements after Gaussian elimination
            // Sign changes from pivoting are implicitly handled if we consider the final diagonal.
            // However, the above loop doesn't track sign changes for the determinant formula.
            // For simplicity and to keep current structure, using Laplace expansion:
            // Reverting to original Laplace expansion as it's what was there, though less stable/efficient
            det = T(0); // Reset det
			for (size_t i = 0; i < n; ++i) {
				Matrix<T> minor = getMinor(*this, 0, i);
				T minor_det = minor.Determinant(); // Recursive call
				int sign = ((i % 2) == 0) ? 1 : -1;
				det += static_cast<T>(sign) * m_Data[0][i] * minor_det;
			}
			return det;
		}

		Matrix<T> Inverse() const {
			if (m_Rows != m_Cols)
				throw std::invalid_argument("Matrix must be square to be inverted.");
            if (m_Rows == 0) return Matrix<T>(); // Inverse of empty is empty

			T det = Determinant();
			if (std::abs(det) < 1e-9) // Check for near-zero determinant for floating point types
				throw std::runtime_error("Matrix is singular (or nearly singular) and cannot be inverted.");

			Matrix<T> cofactors(m_Rows, m_Cols);
			for (size_t i = 0; i < m_Rows; ++i) {
				for (size_t j = 0; j < m_Cols; ++j) {
					Matrix<T> minor = getMinor(*this, i, j);
					T minor_det = minor.Determinant();
					cofactors[i][j] = (((i + j) % 2 == 0) ? T(1) : T(-1)) * minor_det;
				}
			}
			Matrix<T> adjugate = cofactors.Transpose();
			return adjugate * (T(1) / det);
		}

	MatrixRow<T>& operator[](size_t i) {
        // No bounds check for performance in release, but useful for debug
        // #ifndef NDEBUG
        // if (i >= m_Rows) throw std::out_of_range("Matrix row index out of range");
        // #endif
		return m_Data[i];
	}
	const MatrixRow<T>& operator[](size_t i) const {
        // #ifndef NDEBUG
        // if (i >= m_Rows) throw std::out_of_range("Matrix row index out of range");
        // #endif
        return m_Data[i];
    }

	Matrix<T> operator+(const Matrix<T> &b) const { // Keep const for this operator
		if (m_Rows != b.m_Rows || m_Cols != b.m_Cols) {
            if (empty() && !b.empty()) return b; // Adding empty to b returns b
            if (!empty() && b.empty()) return *this; // Adding b (empty) to this returns this
            if (empty() && b.empty()) return Matrix<T>(); // Adding two empty matrices
			throw std::invalid_argument("Matrix dimensions must match for addition.");
        }
		Matrix<T> c(m_Rows, m_Cols);
		for (size_t i = 0; i < m_Rows; i++)
			for (size_t j = 0; j < m_Cols; j++)
				c[i][j] = m_Data[i][j] + b[i][j];
		return c;
	}
	Matrix<T> operator+(const T& val) const { // const T&
		Matrix<T> c(m_Rows, m_Cols);
		for (size_t i = 0; i < m_Rows; i++)
			for (size_t j = 0; j < m_Cols; j++)
				c[i][j] = m_Data[i][j] + val;
		return c;
	}

	Matrix<T>& operator+=(const Matrix<T> &b) { // Not const, return ref
		if (m_Rows != b.m_Rows || m_Cols != b.m_Cols)
			throw std::invalid_argument("Matrix dimensions must match for compound addition.");
		for (size_t i = 0; i < m_Rows; i++)
			for (size_t j = 0; j < m_Cols; j++)
				m_Data[i][j] += b[i][j];
		return *this;
	}

	Matrix<T>& operator+=(const T& val) { // Not const, return ref, const T&
		for (size_t i = 0; i < m_Rows; i++)
			for (size_t j = 0; j < m_Cols; j++)
				m_Data[i][j] += val;
		return *this;
	}

	Matrix<T> operator-(const Matrix<T> &b) const { // Keep const
		if (m_Rows != b.m_Rows || m_Cols != b.m_Cols) {
            if (empty() && !b.empty()) { // Subtracting b from empty
                Matrix<T> neg_b(b.rows(), b.cols());
                for(size_t r=0; r<b.rows(); ++r) for(size_t c=0; c<b.cols(); ++c) neg_b[r][c] = -b[r][c];
                return neg_b;
            }
            if (!empty() && b.empty()) return *this;
            if (empty() && b.empty()) return Matrix<T>();
			throw std::invalid_argument("Matrix dimensions must match for subtraction.");
        }
		Matrix<T> c(m_Rows, m_Cols);
		for (size_t i = 0; i < m_Rows; i++)
			for (size_t j = 0; j < m_Cols; j++)
				c[i][j] = m_Data[i][j] - b[i][j];
		return c;
	}

	Matrix<T> operator-(const T& val) const { // Keep const, const T&
		Matrix<T> c(m_Rows, m_Cols);
		for (size_t i = 0; i < m_Rows; i++)
			for (size_t j = 0; j < m_Cols; j++)
				c[i][j] = m_Data[i][j] - val;
		return c;
	}

	Matrix<T>& operator-=(const Matrix<T> &b) { // Not const, return ref
		if (m_Rows != b.m_Rows || m_Cols != b.m_Cols)
			throw std::invalid_argument("Matrix dimensions must match for compound subtraction.");
		for (size_t i = 0; i < m_Rows; i++)
			for (size_t j = 0; j < m_Cols; j++)
				m_Data[i][j] -= b[i][j];
		return *this;
	}
	Matrix<T>& operator-=(const T& val) { // Not const, return ref, const T&
		for (size_t i = 0; i < m_Rows; i++)
			for (size_t j = 0; j < m_Cols; j++)
				m_Data[i][j] -= val;
		return *this;
	}

	Matrix<T> operator/(const T& val) const { // Keep const, const T&
        if (std::abs(val) < 1e-9) { // Check for division by zero or near-zero for floating points
             throw std::runtime_error("Division by zero or near-zero scalar.");
        }
		Matrix<T> c(m_Rows, m_Cols);
		for (size_t i = 0; i < m_Rows; i++)
			for (size_t j = 0; j < m_Cols; j++)
				c[i][j] = m_Data[i][j] / val;
		return c;
	}

	Matrix<T>& operator/=(const T& val) { // Not const, return ref, const T&
        if (std::abs(val) < 1e-9) {
             throw std::runtime_error("Division by zero or near-zero scalar.");
        }
		for (size_t i = 0; i < m_Rows; i++)
			for (size_t j = 0; j < m_Cols; j++)
				m_Data[i][j] /= val;
		return *this;
	}

	Matrix<T> operator*(const Matrix<T> &b) const { // Keep const
		if (m_Cols != b.m_Rows) {
            if (empty() || b.empty()) return Matrix<T>(m_Rows, b.m_Cols); // Product with empty matrix
            throw std::invalid_argument("Inner dimensions must match for matrix multiplication.");
        }
		Matrix<T> c(m_Rows, b.m_Cols); // Already initialized to zeros by Matrix constructor if T is numeric
		for (size_t i = 0; i < m_Rows; i++)
			for (size_t k = 0; k < b.m_Cols; k++) // iterate over columns of b for result column
				for (size_t j = 0; j < m_Cols; j++) // iterate over columns of this (rows of b)
					c[i][k] += m_Data[i][j] * b[j][k]; // Corrected accumulation and access to b
		return c;
	}

	Matrix<T> operator*(const T& val) const { // Keep const, const T&
		Matrix<T> c(m_Rows, m_Cols);
		for (size_t i = 0; i < m_Rows; i++)
			for (size_t j = 0; j < m_Cols; j++)
				c[i][j] = m_Data[i][j] * val;
		return c;
	}

	Matrix<T>& operator*=(const T& val) { // Not const, return ref, const T&
		for (size_t i = 0; i < m_Rows; i++)
			for (size_t j = 0; j < m_Cols; j++)
				m_Data[i][j] *= val;
		return *this;
	}

	Iterator begin() { return Iterator(m_Data.get()); } // Use .get() for unique_ptr
	Iterator end() { return Iterator(m_Data.get() + m_Rows); } // Use .get()
    Iterator begin() const { return Iterator(m_Data.get()); } // Const version
	Iterator end() const { return Iterator(m_Data.get() + m_Rows); } // Const version


private:
	Matrix<T> getMinor(const Matrix<T> &matrix, size_t row_to_remove, size_t col_to_remove) const {
		if (matrix.m_Rows == 0 || matrix.m_Cols == 0) 
            throw std::invalid_argument("Cannot get minor of an empty matrix.");
        if (matrix.m_Rows != matrix.m_Cols)
			throw std::invalid_argument("Matrix must be square to compute minor.");
        if (matrix.m_Rows < 1) // Should be caught by m_Rows == 0 earlier
             throw std::invalid_argument("Matrix too small to compute minor.");


		size_t n = matrix.m_Rows;
        if (n == 1 && (row_to_remove == 0 && col_to_remove == 0)) { // Minor of 1x1 is 0x0 matrix (det is 1)
            return Matrix<T>(0,0); 
        }
        if (n==0) return Matrix<T>(0,0);


		Matrix<T> minor_matrix(n - 1, n - 1);
		size_t minor_i = 0;
		for (size_t i = 0; i < n; ++i) {
			if (i == row_to_remove)
				continue;
			size_t minor_j = 0;
			for (size_t j = 0; j < n; ++j) {
				if (j == col_to_remove)
					continue;
				minor_matrix[minor_i][minor_j] = matrix.m_Data[i][j];
				++minor_j;
			}
			++minor_i;
		}
		return minor_matrix;
	}

	size_t m_Rows = 0;
	size_t m_Cols = 0;
	size_t m_Size = 0;
	size_t m_Capacity = 0; // Represents number of rows m_Data can hold.
	std::unique_ptr<MatrixRow<T>[]> m_Data;
};
}
#endif
