# Matrix Library Documentation

## Introduction

The **Matrix** library provides a set of classes and iterators for creating and manipulating matrices in C++. It includes implementations for matrix rows, columns, and various operations such as addition, subtraction, multiplication, and more. The library is templated, allowing it to work with any data type that supports the necessary arithmetic operations.

## Namespace

- **`Matrix`**: The primary namespace containing all the classes and functions related to matrix operations.

## Classes Overview

1. **`MatrixRowIterator<MatrixRow>`**: An iterator for traversing elements within a matrix row.
2. **`MatrixColumnIterator<T>`**: An iterator for traversing elements within a matrix column (by stepping over `totalColumns` elements).
3. **`MatrixIterator<Matrix>`**: An iterator for traversing rows (`MatrixRow<T>`) within a `Matrix<T>`.
4. **`MatrixRow<T>`**: Represents a single row in a matrix, managing its own data.
5. **`Matrix<T>`**: Represents a two-dimensional matrix composed of `MatrixRow<T>` objects, managing a collection of rows.

---

## Class Details

### 1. `MatrixRowIterator<MatrixRow>`

#### Description

A template class that provides a random-access iterator for iterating over the elements of a `MatrixRow`. It conforms to standard iterator requirements, making it compatible with STL algorithms and range-based for loops.

#### Member Types

- `value_type`: Type of elements the iterator refers to (`typename MatrixRowType::value_type`).
- `pointer`: Pointer to the element type (`value_type*`).
- `reference`: Reference to the element type (`value_type&`).
- `iterator_category`: Iterator category (`std::random_access_iterator_tag`).
- `difference_type`: Type to express the difference between two iterators (`std::ptrdiff_t`).

#### Member Functions

- **Constructor**
  - `MatrixRowIterator(pointer ptr)`: Initializes the iterator with a pointer to a matrix row element.

- **Increment Operators**
  - `MatrixRowIterator& operator++()`: Pre-increment operator.
  - `MatrixRowIterator operator++(int)`: Post-increment operator.

- **Arithmetic Operators**
  - `MatrixRowIterator operator+(difference_type n) const`: Advances the iterator by `n` positions.
  - `MatrixRowIterator& operator+=(difference_type n)`: Advances the iterator by `n` positions (compound assignment).

- **Decrement Operators**
  - `MatrixRowIterator& operator--()`: Pre-decrement operator.
  - `MatrixRowIterator operator--(int)`: Post-decrement operator.

- **Reverse Arithmetic Operators**
  - `MatrixRowIterator operator-(difference_type n) const`: Moves the iterator back by `n` positions.
  - `MatrixRowIterator& operator-=(difference_type n)`: Moves the iterator back by `n` positions (compound assignment).
  - `difference_type operator-(const MatrixRowIterator& other) const`: Calculates the difference between two iterators.

- **Dereference Operators**
  - `reference operator*()`: Dereferences the iterator to access the element (non-const).
  - `pointer operator->() const`: Accesses members of the element. (Note: `m_ptr` itself is returned, allowing modification if `value_type` is not const).
  - `reference operator[](difference_type n) const`: Accesses the element at an offset `n` from the current position.

- **Comparison Operators**
  - `bool operator==(const MatrixRowIterator& other) const`: Equality comparison.
  - `bool operator!=(const MatrixRowIterator& other) const`: Inequality comparison.
  - `bool operator<(const MatrixRowIterator& other) const`: Less-than comparison.
  - `bool operator<=(const MatrixRowIterator& other) const`: Less-than-or-equal comparison.
  - `bool operator>(const MatrixRowIterator& other) const`: Greater-than comparison.
  - `bool operator>=(const MatrixRowIterator& other) const`: Greater-than-or-equal comparison.
(Note: The `MatrixRow` class provides `begin() const` and `end() const` which return `MatrixRowIterator`. If `MatrixRowIterator` is used with a const `MatrixRow`, `operator*` would effectively yield a const reference due to const propagation from `m_Data.get()`).

---

### 2. `MatrixColumnIterator<T>`

#### Description

A template class that provides a random-access iterator for iterating over the elements of a matrix column. It moves the internal pointer in steps equal to the total number of columns, allowing column-wise traversal.

#### Member Types

- `value_type`: Type of elements the iterator can dereference (`T`).
- `pointer`: Pointer to the element type (`T*`).
- `reference`: Reference to the element type (`T&`).
- `iterator_category`: Iterator category (`std::random_access_iterator_tag`).
- `difference_type`: Type to express the difference between two iterators (`std::ptrdiff_t`).

#### Member Functions

- **Constructor**
  - `MatrixColumnIterator(pointer ptr, size_t totalColumns)`: Initializes the iterator with a pointer to a matrix element and the total number of columns.

- **Increment Operators**
  - `MatrixColumnIterator& operator++()`: Pre-increment operator (moves down one row).
  - `MatrixColumnIterator operator++(int)`: Post-increment operator.

- **Arithmetic Operators**
  - `MatrixColumnIterator operator+(difference_type n) const`: Advances the iterator by `n` positions down the column.
  - `MatrixColumnIterator& operator+=(difference_type n)`: Advances the iterator by `n` positions (compound assignment).

- **Decrement Operators**
  - `MatrixColumnIterator& operator--()`: Pre-decrement operator (moves up one row).
  - `MatrixColumnIterator operator--(int)`: Post-decrement operator.

- **Reverse Arithmetic Operators**
  - `MatrixColumnIterator operator-(difference_type n) const`: Moves the iterator back by `n` positions up the column.
  - `MatrixColumnIterator& operator-=(difference_type n)`: Moves the iterator back by `n` positions (compound assignment).
  - `difference_type operator-(const MatrixColumnIterator& other) const`: Calculates the difference between two iterators in terms of column positions.

- **Dereference Operators**
  - `reference operator*() const`: Dereferences the iterator to access the element.
  - `pointer operator->() const`: Accesses members of the element.
  - `reference operator[](difference_type n) const`: Accesses the element at an offset `n` from the current position.

- **Comparison Operators**
  - `bool operator==(const MatrixColumnIterator& other) const`: Equality comparison.
  - `bool operator!=(const MatrixColumnIterator& other) const`: Inequality comparison.
  - `bool operator<(const MatrixColumnIterator& other) const`: Less-than comparison.
  - `bool operator<=(const MatrixColumnIterator& other) const`: Less-than-or-equal comparison.
  - `bool operator>(const MatrixColumnIterator& other) const`: Greater-than comparison.
  - `bool operator>=(const MatrixColumnIterator& other) const`: Greater-than-or-equal comparison.

---

### 3. `MatrixIterator<Matrix>`

#### Description

A template class that provides an iterator for iterating over the rows of a matrix. It allows traversal of the matrix at the row level.

#### Member Types

- `value_type`: Type of elements (rows) the iterator refers to (`typename MatrixType::value_type`, which is `MatrixRow<T>`).
- `pointer`: Pointer to the element type (`value_type*`).
- `reference`: Reference to the element type (`value_type&`).
- `iterator_category`: Typically `std::random_access_iterator_tag` if full support is implemented, otherwise `std::input_iterator_tag` or `std::forward_iterator_tag`. (Header does not specify, but operations imply random access).
- `difference_type`: Type to express the difference between two iterators (`std::ptrdiff_t`).


#### Member Functions

- **Constructor**
  - `MatrixIterator(pointer ptr)`: Initializes the iterator with a pointer to a matrix row.

- **Increment Operators**
  - `MatrixIterator& operator++()`: Pre-increment operator (moves to the next row).
  - `MatrixIterator operator++(int)`: Post-increment operator.

- **Decrement Operators**
  - `MatrixIterator& operator--()`: Pre-decrement operator (moves to the previous row).
  - `MatrixIterator operator--(int)`: Post-decrement operator.

- **Dereference Operators**
  - `reference operator*()`: Dereferences the iterator to access the row.
  - `pointer operator->()`: Accesses members of the row.

- **Comparison Operators**
  - `bool operator==(const MatrixIterator& other) const`: Equality comparison.
  - `bool operator!=(const MatrixIterator& other) const`: Inequality comparison.
  (Note: `<, <=, >, >=` operators are not explicitly defined in the header for `MatrixIterator`.)

---

### 4. `MatrixRow<T>`

#### Description

A template class representing a single row in a matrix. It manages a dynamic array of elements of type `T` and provides functionalities for resizing and accessing elements within the row.

#### Member Types

- `value_type`: Type of elements stored in the row (`T`).
- `Iterator`: An iterator type (`MatrixRowIterator<MatrixRow<T>>`) for iterating over row elements.

#### Constructors

- `MatrixRow()`: Default constructor. Initializes an empty row with size and capacity 0.
- `explicit MatrixRow(size_t size)`: Constructs a `MatrixRow` with the specified `size`. Elements are default initialized (e.g., to 0 for numeric types).
- `MatrixRow(const MatrixRow& other)`: Copy constructor. Performs a deep copy of the other row's elements.
- `MatrixRow(MatrixRow&& other) noexcept`: Move constructor. Takes ownership of the other row's data.

#### Assignment Operators
- `MatrixRow& operator=(const MatrixRow& other)`: Copy assignment operator. Performs a deep copy.
- `MatrixRow& operator=(MatrixRow&& other) noexcept`: Move assignment operator. Takes ownership of the other row's data.

#### Destructor
- `~MatrixRow()`: Defaulted destructor, handles memory deallocation via `std::unique_ptr`.


#### Member Functions

- **Resizing and Assignment**
  - `void resize(size_t newSize)`: Resizes the row to `newSize`. If `newSize` is smaller, elements are truncated. If larger, new elements are default initialized.
  - `void assign(size_t size, T val)`: Resizes the row to `size` and assigns the value `val` to all elements.
  - `void assign(T val)`: Assigns the value `val` to all existing elements without changing the size.

- **Accessors**
  - `size_t size() const`: Returns the number of elements in the row.
  - `size_t capacity() const`: Returns the current capacity of the row (typically same as size in this implementation).
  - `T at(size_t i) const`: Returns the element at index `i`, with bounds checking (throws `std::out_of_range` if `i` is invalid).
  - `T& at(size_t i)`: Returns a reference to the element at index `i`, with bounds checking.

- **Operators**
  - `T& operator[](size_t i)`: Accesses the element at index `i`. **No bounds checking is performed.**
  - `const T& operator[](size_t i) const`: Const version. **No bounds checking is performed.**

- **Iterators**
  - `Iterator begin()`: Returns an iterator to the beginning of the row.
  - `Iterator end()`: Returns an iterator to the end of the row.
  - `Iterator begin() const`: Const version, returns an iterator to the beginning of a const row.
  - `Iterator end() const`: Const version, returns an iterator to the end of a const row.

---

### 5. `Matrix<T>`

#### Description

A template class representing a two-dimensional matrix composed of `MatrixRow<T>` objects. It provides a wide range of functionalities including resizing, arithmetic operations, matrix manipulation (transpose, merge, split), and more.

#### Member Types

- `value_type`: Type of rows stored in the matrix (`MatrixRow<T>`).
- `Iterator`: An iterator type (`MatrixIterator<Matrix<T>>`) for iterating over matrix rows.
- `ColumnIterator`: An iterator type (`MatrixColumnIterator<T>`) for iterating over elements in a specific column (conceptual, not directly provided as `begin_col`/`end_col` by `Matrix` class itself).

#### Constructors

- `Matrix()`: Default constructor. Initializes an empty matrix (0 rows, 0 columns).
- `explicit Matrix(size_t row_count, size_t column_count)`: Constructs a matrix with the specified number of rows and columns. Elements are default initialized.
- `Matrix(size_t row_count, size_t column_count, const T& initial_value)`: Constructs a matrix and initializes all elements to `initial_value`.
- `Matrix(const Matrix& other)`: Copy constructor. Performs a deep copy.
- `Matrix(Matrix&& other) noexcept`: Move constructor. Takes ownership of `other`'s data.

#### Assignment Operators
- `Matrix& operator=(const Matrix& other)`: Copy assignment operator. Performs a deep copy.
- `Matrix& operator=(Matrix&& other) noexcept`: Move assignment operator. Takes ownership of `other`'s data.

#### Destructor
- `~Matrix()`: Defaulted destructor, handles memory deallocation via `std::unique_ptr`.

#### Member Functions

- **Accessors**
  - `size_t size() const`: Returns the total number of elements in the matrix (rows * columns).
  - `size_t rows() const`: Returns the number of rows.
  - `size_t cols() const`: Returns the number of columns.
  - `size_t capacity() const`: Returns the capacity of the matrix in terms of the number of rows it can store without reallocation of the main row array.
  - `bool empty() const`: Returns `true` if the matrix has 0 rows or 0 columns, `false` otherwise.
  - `T at(size_t r, size_t c) const`: Returns the element at row `r` and column `c`, with bounds checking.
  - `T& at(size_t r, size_t c)`: Returns a reference to the element at row `r` and column `c`, with bounds checking.

- **Resizing and Assignment**
  - `void resize(size_t row_count, size_t col_count)`: Resizes the matrix. Existing elements that fit are preserved. New elements are default initialized.
  - `void resize(size_t row_count, size_t col_count, const T& val)`: Resizes the matrix. Existing elements that fit are preserved. New elements are initialized to `val`. Other elements are assigned `val`.
  - `void assign(size_t row_count, size_t col_count, const T& val)`: Resizes the matrix and assigns `val` to all its elements.
  - `void assign(const T& val)`: Assigns `val` to all existing elements without changing dimensions.

- **Matrix Manipulation**
  - `Matrix<T> MergeVertical(const Matrix<T>& b) const`: Merges the current matrix with matrix `b` vertically (appends rows of `b`). Matrices must have the same number of columns, unless one is empty.
  - `Matrix<T> MergeHorizontal(const Matrix<T>& b) const`: Merges the current matrix with matrix `b` horizontally (appends columns of `b`). Matrices must have the same number of rows, unless one is empty.
  - `std::vector<Matrix<T>> SplitVertical(size_t num_splits = 2) const`: Splits the matrix vertically into `num_splits` parts. Number of rows must be divisible by `num_splits`.
  - `std::vector<Matrix<T>> SplitHorizontal(size_t num_splits = 2) const`: Splits the matrix horizontally into `num_splits` parts. Number of columns must be divisible by `num_splits`.
  - `Matrix<T>& SigmoidMatrix()`: Applies the sigmoid function `1 / (1 + exp(-x))` to each element of the matrix, modifying it in-place. Returns `*this`.
  - `Matrix<T>& Randomize()`: Randomizes the elements of the matrix with values typically between -1.0 and 1.0 (uses `std::uniform_real_distribution`). Modifies in-place. Returns `*this`.
  - `Matrix<T>& CreateIdentityMatrix()`: Converts the matrix into an identity matrix (must be square). Modifies in-place. Returns `*this`.
  - `Matrix<T>& ZeroMatrix()`: Sets all elements to zero. Modifies in-place. Returns `*this`.
  - `Matrix<T> Transpose() const`: Returns the transpose of the matrix.
  - `T Determinant() const`: Returns the determinant of the matrix (must be square). Uses Laplace expansion (recursive).
  - `Matrix<T> Inverse() const`: Returns the inverse of the matrix (must be square and non-singular).

- **Operators**
  - `MatrixRow<T>& operator[](size_t i)`: Accesses the row at index `i`. **No bounds checking.**
  - `const MatrixRow<T>& operator[](size_t i) const`: Const version. **No bounds checking.**

  - **Arithmetic Operators**
    - **Addition**
      - `Matrix<T> operator+(const Matrix<T>& b) const`: Adds two matrices element-wise.
      - `Matrix<T> operator+(const T& b) const`: Adds a scalar `b` to each element.
      - `Matrix<T>& operator+=(const Matrix<T>& b)`: Adds another matrix to this matrix (in-place).
      - `Matrix<T>& operator+=(const T& b)`: Adds a scalar `b` to each element (in-place).
    - **Subtraction**
      - `Matrix<T> operator-(const Matrix<T>& b) const`: Subtracts another matrix from this matrix element-wise.
      - `Matrix<T> operator-(const T& b) const`: Subtracts a scalar `b` from each element.
      - `Matrix<T>& operator-=(const Matrix<T>& b)`: Subtracts another matrix from this matrix (in-place).
      - `Matrix<T>& operator-=(const T& b)`: Subtracts a scalar `b` from each element (in-place).
    - **Multiplication**
      - `Matrix<T> operator*(const Matrix<T>& b) const`: Multiplies two matrices (matrix multiplication). Requires `this->cols() == b.rows()`.
      - `Matrix<T> operator*(const T& b) const`: Multiplies each element by a scalar `b`.
      - `Matrix<T>& operator*=(const T& b)`: Multiplies each element by a scalar `b` (in-place).
    - **Division**
      - `Matrix<T> operator/(const T& b) const`: Divides each element by a scalar `b`. Throws `std::runtime_error` for division by zero.
      - `Matrix<T>& operator/=(const T& b)`: Divides each element by a scalar `b` (in-place). Throws `std::runtime_error` for division by zero.

- **Iterators**
  - `Iterator begin()`: Returns an iterator to the beginning of the matrix (first row).
  - `Iterator end()`: Returns an iterator to the end of the matrix (one past the last row).
  - `Iterator begin() const`: Const version.
  - `Iterator end() const`: Const version.

---

## Usage Examples

### Example 1: Creating and Initializing a Matrix

```cpp
#include "Matrix.h"

int main() {
    // Create a 3x3 matrix of integers, initialized to default (0 for int)
    Matrix::Matrix<int> mat(3, 3);

    // Assign values to the matrix
    mat.assign(1); // Set all elements to 1 using assign

    // Modify specific elements using operator[] (no bounds check)
    // For bounds-checked access, use mat.at(row, col) = value;
    mat[0][0] = 5;
    mat[1][1] = 10;
    mat[2][2] = 15;

    return 0;
}

```

### Example 2: Matrix Addition

```cpp
#include "Matrix.h"

int main() {
    Matrix::Matrix<int> mat1(3, 3);
    Matrix::Matrix<int> mat2(3, 3);

    // Initialize matrices
    mat1.assign(2);
    mat2.assign(3);

    // Add matrices
    Matrix::Matrix<int> result = mat1 + mat2;

    // Output the result
    for (size_t i = 0; i < result.rows(); ++i) {
        for (size_t j = 0; j < result.cols(); ++j) {
            std::cout << result[i][j] << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}
 ```

 ### Example 3: Matrix Multiplication

 ```cpp
 #include "Matrix.h"

int main() {
    Matrix::Matrix<int> mat1(2, 3);
    Matrix::Matrix<int> mat2(3, 2);

    // Initialize matrices with random values
    // Initialize matrices (Randomize modifies in-place and returns a reference)
    mat1.Randomize();
    mat2.Randomize();

    // Multiply matrices
    Matrix::Matrix<int> result = mat1 * mat2; // mat1.operator*(mat2)

    // Output the result
    // Using operator[] (no bounds check). For bounds-checked access, use result.at(i,j)
    for (size_t i = 0; i < result.rows(); ++i) {
        for (size_t j = 0; j < result.cols(); ++j) {
            std::cout << result[i][j] << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}
```

## Notes
 - Type Requirements: The Matrix class assumes that the type `T` supports basic arithmetic operations (e.g., `+`, `-`, `*`, `/`), copy/move construction/assignment, and default construction. For functions like `Determinant`, `Inverse`, `SigmoidMatrix`, specific properties (e.g., being a numeric type) are implicitly required.
 - Exception Handling: Exception handling is implemented for index bounds checking in `at()` methods, invalid operations (e.g., merging matrices with incompatible dimensions, determinant of non-square matrix, division by zero), and memory allocation failures (implicitly via `std::unique_ptr`).
 - Modern C++ Features: The code uses modern C++ features such as smart pointers (`std::unique_ptr`), templates, move semantics (constructors and assignment operators), and the Standard Template Library (STL) (e.g., `std::vector`, `std::algorithm`, `std::iterator`).
 - Performance: `operator[]` for `MatrixRow` and `Matrix` do not perform bounds checking for performance reasons, similar to `std::vector::operator[]`. Use the `at()` methods for safe, bounds-checked access.

## Conclusion

The Matrix library provides a flexible and efficient way to work with matrices in C++. With support for various matrix operations, custom iterators, and modern C++ memory management, it can be integrated into applications that require matrix computations, such as scientific computing, graphics, and machine learning.

## Potential Improvements
- **Error Handling**: Enhance exception messages for even more clarity and context.
- **Performance Optimizations**:
    - Explore expression templates to reduce temporary object creation in chained operations.
    - For performance-critical applications, investigate SIMD optimizations for arithmetic operations.
- **Additional Functions**:
    - Eigenvalue and eigenvector calculations.
    - Decompositions like QR, SVD.
    - More specialized matrix types (e.g., SparseMatrix, SymmetricMatrix).
- **Template Specializations**: Provide explicit specializations for common types (e.g., `float`, `double`) if specific optimized algorithms for those types are beneficial.
- **Iterator Enhancements**:
    - Ensure all iterators fully conform to `std::random_access_iterator_tag` requirements if not already the case (e.g., `MatrixIterator` comparison operators).
    - Consider adding `const_iterator` types explicitly for `MatrixRow` and `Matrix` for more C++ idiomatic const-correctness, although `begin() const` returning a non-const iterator type that operates on const data is a common pattern.
- **Concurrency**: Add support for parallelized operations for large matrices.

## References
- C++ Standard Library: Utilizes features from the C++ Standard Library, including iterators, smart pointers, algorithms, and exception classes.
- Iterator Requirements: Aims to conform to C++ iterator requirements for compatibility with STL algorithms.

