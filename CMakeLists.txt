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

# Add your main library
add_library(bitmap STATIC src/bitmap.cpp )

# Executable for main.cpp (if it's a demo or separate utility)
add_executable(testexe main.cpp)
target_link_libraries(testexe PRIVATE bitmap bitmapfile matrix) # Link testexe against our libraries

# GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

# Add test executable
# It needs to compile the test source file, the bitmap source file, and the dependency's source file
add_executable(bitmap_tests tests/test_bitmap.cpp src/bitmap.cpp dependencies/bitmapfile/src/bitmap_file.cpp)
target_link_libraries(bitmap_tests PRIVATE bitmap bitmapfile matrix gtest_main)

# Add test to CTest
add_test(NAME BitmapUnitTests COMMAND bitmap_tests --gtest_color=yes)
