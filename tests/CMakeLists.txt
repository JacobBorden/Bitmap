# CMakeLists.txt for tests directory using GTest and CTest's test discovery
cmake_minimum_required(VERSION 3.10)

# Enable testing
enable_testing()

# Add test sources
set(TEST_SOURCES
    test_bitmap.cpp
    test_bitmap_file.cpp
    test_matrix.cpp
    tests_main.cpp
    api_roundtrip.cpp
)

# Create test executable
add_executable(bitmap_tests ${TEST_SOURCES})

target_include_directories(bitmap_tests PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Link against main bitmap library and gtest only (bitmapfile and matrix are now part of bitmap)
target_link_libraries(bitmap_tests PRIVATE bitmap gtest_main)

# Use CTest's test discovery for GoogleTest
include(GoogleTest)
gtest_discover_tests(bitmap_tests)
