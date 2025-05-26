cmake_minimum_required(VERSION 3.10)
project(bitmap VERSION 1.0.0)

include(CTest)
enable_testing()

# Add your main library with all sources
add_library(bitmap STATIC 
    src/bitmap/bitmap.cpp
    src/bitmapfile/bitmap_file.cpp
)

# Executable for main.cpp (if it's a demo or separate utility)
add_executable(testexe main.cpp)
target_link_libraries(testexe PRIVATE bitmap)

target_include_directories(bitmap PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bitmapfile
    ${CMAKE_CURRENT_SOURCE_DIR}/src/matrix
)

# GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

# Add subdirectory for tests (enables gtest_discover_tests and test auto-discovery)
add_subdirectory(tests)
