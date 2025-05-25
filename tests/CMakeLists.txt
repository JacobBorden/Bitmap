# CMakeLists.txt for tests directory using GTest and CTest's test discovery
cmake_minimum_required(VERSION 3.10)

# Enable testing
enable_testing()

# Add test sources
set(TEST_SOURCES
    test_bitmap.cpp
    tests_main.cpp
)

# Create test executable
add_executable(bitmap_tests ${TEST_SOURCES})

# Link against main bitmap library and dependencies
# (Assumes bitmap, bitmapfile, matrix, and gtest_main are available in parent scope)
target_link_libraries(bitmap_tests PRIVATE bitmap bitmapfile matrix gtest_main)

# Use CTest's test discovery for GoogleTest
include(GoogleTest)
gtest_discover_tests(bitmap_tests)
