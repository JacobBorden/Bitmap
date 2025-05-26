# Matrix Library Documentation

## Introduction

The **Matrix** library provides a set of classes and iterators for creating and manipulating matrices in C++. It includes implementations for matrix rows, columns, and various operations such as addition, subtraction, multiplication, and more. The library is templated, allowing it to work with any data type that supports the necessary arithmetic operations.

## Namespace

- **`Matrix`**: The primary namespace containing all the classes and functions related to matrix operations.

## Classes Overview

1. **`MatrixRowIterator<MatrixRow>`**: An iterator for traversing elements within a matrix row.
2. **`MatrixColumnIterator<T>`**: An iterator for traversing elements within a matrix column.
3. **`MatrixIterator<Matrix>`**: An iterator for traversing rows within a matrix.
4. **`MatrixRow<T>`**: Represents a single row in a matrix.
5. **`Matrix<T>`**: Represents a two-dimensional matrix composed of `MatrixRow<T>` objects.

---

## Class Details

### 1. `MatrixRowIterator<MatrixRow>`

#### Description

A template class that provides a random-access iterator for iterating over the elements of a `MatrixRow`. It conforms to standard iterator requirements, making it compatible with STL algorithms and range-based for loops.

#### Member Types

- `value_type`: Type of elements the iterator refers to (`typename MatrixRow::value_type`).
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
  - `reference operator*()`: Dereferences the iterator to access the element.
  - `const reference operator*() const`: Const version.
  - `pointer operator->() const`: Accesses members of the element.
  - `reference operator[](difference_type n) const`: Accesses the element at an offset `n` from the current position.

- **Comparison Operators**
  - `bool operator==(const MatrixRowIterator& other) const`: Equality comparison.
  - `bool operator!=(const MatrixRowIterator& other) const`: Inequality comparison.
  - `bool operator<(const MatrixRowIterator& other) const`: Less-than comparison.
  - `bool operator<=(const MatrixRowIterator& other) const`: Less-than-or-equal comparison.
  - `bool operator>(const MatrixRowIterator& other) const`: Greater-than comparison.
  - `bool operator>=(const MatrixRowIterator& other) const`: Greater-than-or-equal comparison.

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

- `value_type`: Type of elements (rows) the iterator refers to (`typename Matrix::value_type`).
- `pointer`: Pointer to the element type (`value_type*`).
- `reference`: Reference to the element type (`value_type&`).

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
  - `bool operator==(MatrixIterator other)`: Equality comparison.
  - `bool operator!=(MatrixIterator other)`: Inequality comparison.

---

### 4. `MatrixRow<T>`

#### Description

A template class representing a single row in a matrix. It manages a dynamic array of elements of type `T` and provides functionalities for resizing and accessing elements within the row.

#### Member Types

- `value_type`: Type of elements stored in the row (`T`).
- `Iterator`: An iterator type (`MatrixRowIterator<MatrixRow<T>>`) for iterating over row elements.

#### Constructors

- `MatrixRow()`: Default constructor.
- `explicit MatrixRow(size_t size)`: Constructs a `MatrixRow` with the specified size.

#### Member Functions

- **Resizing and Assignment**
  - `void resize(size_t newSize)`: Resizes the row to `newSize`.
  - `void assign(size_t size, T val)`: Assigns the value `val` to all elements, resizing the row to `size`.
  - `void assign(T val)`: Assigns the value `val` to all existing elements.

- **Accessors**
  - `size_t size() const`: Returns the number of elements in the row.
  - `size_t capacity()`: Returns the capacity of the row.
  - `T at(size_t i) const`: Returns the element at index `i`, with bounds checking.

- **Operators**
  - `T& operator[](size_t i)`: Accesses the element at index `i` with bounds checking.
  - `const T& operator[](size_t i) const`: Const version.

- **Iterators**
  - `Iterator begin()`: Returns an iterator to the beginning of the row.
  - `Iterator end()`: Returns an iterator to the end of the row.
  - `Iterator begin() const`: Const version.
  - `Iterator end() const`: Const version.

---

### 5. `Matrix<T>`

#### Description

A template class representing a two-dimensional matrix composed of `MatrixRow<T>` objects. It provides a wide range of functionalities including resizing, arithmetic operations, matrix manipulation (transpose, merge, split), and more.

#### Member Types

- `value_type`: Type of rows stored in the matrix (`MatrixRow<T>`).
- `Iterator`: An iterator type (`MatrixIterator<Matrix<T>>`) for iterating over matrix rows.
- `ColumnIterator`: An iterator type (`MatrixColumnIterator<T>`) for iterating over matrix columns.

#### Constructors

- `Matrix()`: Default constructor.
- `explicit Matrix(int row_count, int column_count)`: Constructs a matrix with the specified number of rows and columns.

#### Member Functions

- **Accessors**
  - `size_t size() const`: Returns the total number of elements in the matrix.
  - `size_t rows() const`: Returns the number of rows.
  - `size_t cols() const`: Returns the number of columns.
  - `size_t capacity() const`: Returns the capacity of the matrix.

- **Resizing and Assignment**
  - `void resize(size_t row_count, size_t col_count)`: Resizes the matrix to the specified dimensions.
  - `void assign(size_t row_count, size_t col_count, const T val)`: Resizes and assigns `val` to all elements.
  - `void assign(const T val)`: Assigns `val` to all existing elements.

- **Matrix Manipulation**
  - `Matrix<T> MergeVertical(const Matrix<T>& b) const`: Merges the current matrix with another matrix `b` vertically.
  - `Matrix<T> MergeHorizontal(const Matrix<T>& b) const`: Merges the current matrix with another matrix `b` horizontally.
  - `std::vector<Matrix<T>> SplitVertical() const`: Splits the matrix vertically into two equal parts.
  - `std::vector<Matrix<T>> SplitVertical(size_t num) const`: Splits the matrix vertically into `num` equal parts.
  - `std::vector<Matrix<T>> SplitHorizontal() const`: Splits the matrix horizontally into two equal parts.
  - `std::vector<Matrix<T>> SplitHorizontal(size_t num) const`: Splits the matrix horizontally into `num` equal parts.
  - `Matrix<T> SigmoidMatrix()`: Applies the sigmoid function to each element of the matrix.
  - `Matrix<T> Randomize()`: Randomizes the elements of the matrix with values between -1.0 and 1.0.
  - `Matrix<T> CreateIdentityMatrix()`: Converts the matrix into an identity matrix (must be square).
  - `Matrix<T> ZeroMatrix() const`: Sets all elements to zero.
  - `Matrix<T> Transpose() const`: Returns the transpose of the matrix.
  - `T Determinant() const`: Returns the determinant of the matrix.
  - `Matrix<T> Inverse()`: Returns the inverse of the matrix.

- **Operators**
  - `MatrixRow<T>& operator[](size_t i)`: Accesses the row at index `i`.
  - `const MatrixRow<T>& operator[](size_t i) const`: Const version.

  - **Arithmetic Operators**
    - **Addition**
      - `Matrix<T> operator+(const Matrix<T>& b)`: Adds two matrices element-wise.
      - `Matrix<T> operator+(const T b) const`: Adds a scalar to each element.
      - `Matrix<T> operator+=(const Matrix<T>& b) const`: Adds another matrix to this matrix.
      - `Matrix<T> operator+=(const T b) const`: Adds a scalar to each element.
    - **Subtraction**
      - `Matrix<T> operator-(const Matrix<T>& b) const`: Subtracts another matrix from this matrix element-wise.
      - `Matrix<T> operator-(const T b) const`: Subtracts a scalar from each element.
      - `Matrix<T> operator-=(const Matrix<T>& b) const`: Subtracts another matrix from this matrix.
      - `Matrix<T> operator-=(const T b) const`: Subtracts a scalar from each element.
    - **Multiplication**
      - `Matrix<T> operator*(const Matrix<T>& b) const`: Multiplies two matrices (matrix multiplication).
      - `Matrix<T> operator*(const T b) const`: Multiplies each element by a scalar.
      - `Matrix<T> operator*=(const T b) const`: Multiplies each element by a scalar.
    - **Division**
      - `Matrix<T> operator/(const T b) const`: Divides each element by a scalar.
      - `Matrix<T> operator/=(const T b) const`: Divides each element by a scalar.

- **Iterators**
  - `Iterator begin()`: Returns an iterator to the beginning of the matrix (rows).
  - `Iterator end()`: Returns an iterator to the end of the matrix.

---

## Usage Examples

### Example 1: Creating and Initializing a Matrix

```cpp
#include "Matrix.h"

int main() {
    // Create a 3x3 matrix of integers
    Matrix::Matrix<int> mat(3, 3);

    // Assign values to the matrix
    mat.assign(1); // Set all elements to 1

    // Modify specific elements
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
    mat1.Randomize();
    mat2.Randomize();

    // Multiply matrices
    Matrix::Matrix<int> result = mat1 * mat2;

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

## Notes
 - Type Requirements: The Matrix class assumes that the type T supports basic arithmetic operations such as addition, subtraction, multiplication, and division.
 - Exception Handling: Exception handling is implemented for index bounds checking and invalid operations (e.g., merging matrices with incompatible dimensions).
 - Modern C++ Features: The code uses modern C++ features such as smart pointers (std::unique_ptr), templates, and the Standard Template Library (STL).

## Conclusion

The Matrix library provides a flexible and efficient way to work with matrices in C++. With support for various matrix operations and custom iterators, it can be integrated into applications that require matrix computations, such as scientific computing, graphics, and machine learning.

## Potential Improvements
- Error Handling: Enhance exception messages for more clarity.
- Optimizations: Implement move semantics where applicable for performance improvements.
- Additional Functions: Add more mathematical operations like determinant calculation, inverse, eigenvalues, etc.
- Template Specializations: Provide specializations for common types (e.g., int, float, double) for optimized performance.

## References
- C++ Standard Library: Utilizes features from the C++ Standard Library, including iterators and smart pointers.
- Iterator Requirements: Conforms to the C++ iterator requirements for compatibility with STL algorithms.

