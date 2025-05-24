cmake_minimum_required(VERSION 3.10)
project(bitmap VERSION 1.0.0)

include(CTest)
enable_testing()
include(FetchContent)

FetchContent_Declare(
    matrix
    GIT_REPOSITORY https://github.com/JacobBorden/Matrix.git
    SOURCE_DIR ../dependencies/matrix
)

FetchContent_Declare(
    bitmapfile
    GIT_REPOSITORY https://github.com/JacobBorden/BitmapFile.git
    SOURCE_DIR ../dependencies/bitmapfile
)

FetchContent_MakeAvailable(bitmapfile)
FetchContent_MakeAvailable(matrix)

link_libraries(matrix)
link_libraries(bitmapfile)
add_library(bitmap STATIC src/bitmap.cpp ) # Main library
link_libraries(bitmap) # Link bitmap library to subsequent targets if needed

# Executable for main.cpp (if it's a demo or separate utility)
add_executable(testexe main.cpp)
target_link_libraries(testexe PRIVATE bitmap bitmapfile matrix) # Link testexe against our libraries

# Add test executable
# It needs to compile the test source file, the bitmap source file, and the dependency's source file
add_executable(bitmap_tests tests/test_bitmap.cpp src/bitmap.cpp dependencies/bitmapfile/src/bitmap_file.cpp)
# Link bitmap_tests against the matrix library (as bitmap.cpp depends on it)
target_link_libraries(bitmap_tests PRIVATE matrix)

# Add test to CTest
add_test(NAME BitmapUnitTests COMMAND bitmap_tests)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set( CPACK_PROJECT_VERSION ${PROJECT_VERSION}) 
include (CPack)