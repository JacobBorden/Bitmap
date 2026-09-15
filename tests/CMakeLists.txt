# CMakeLists.txt for tests directory using GTest and CTest's test discovery
cmake_minimum_required(VERSION 3.10)

# Enable testing
enable_testing()

# Add test sources
set(TEST_SOURCES
    test_bitmap.cpp
    test_bitmap_file.cpp
    test_matrix.cpp
    test_swizzle.cpp # <-- Add this line
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

if(ENABLE_FUZZING AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(FUZZ_LINK_FLAGS "-fsanitize=address,undefined,fuzzer") # Includes 'undefined'

  # Original fuzzer, updated
  add_executable(fuzz_bitmap fuzz/fuzz_bitmap.cpp)
  target_link_libraries(fuzz_bitmap PRIVATE bitmap)
  set_target_properties(fuzz_bitmap PROPERTIES LINK_FLAGS "${FUZZ_LINK_FLAGS}")
  message(STATUS "Fuzz target fuzz_bitmap (updated flags) added.")

  # New fuzzer for BmpTool::save
  add_executable(fuzz_bmp_tool_save fuzz/fuzz_bmp_tool_save.cpp)
  target_link_libraries(fuzz_bmp_tool_save PRIVATE bitmap)
  set_target_properties(fuzz_bmp_tool_save PROPERTIES LINK_FLAGS "${FUZZ_LINK_FLAGS}")
  message(STATUS "Fuzz target fuzz_bmp_tool_save added.")

  # New fuzzer for Bitmap::File operations
  add_executable(fuzz_bitmap_file fuzz/fuzz_bitmap_file.cpp)
  target_link_libraries(fuzz_bitmap_file PRIVATE bitmap)
  set_target_properties(fuzz_bitmap_file PROPERTIES LINK_FLAGS "${FUZZ_LINK_FLAGS}")
  message(STATUS "Fuzz target fuzz_bitmap_file added.")

  # New fuzzer for image manipulation functions
  add_executable(fuzz_image_operations fuzz/fuzz_image_operations.cpp)
  target_link_libraries(fuzz_image_operations PRIVATE bitmap)
  set_target_properties(fuzz_image_operations PROPERTIES LINK_FLAGS "${FUZZ_LINK_FLAGS}")
  message(STATUS "Fuzz target fuzz_image_operations added.")

  # New fuzzer for Matrix operations
  add_executable(fuzz_matrix fuzz/fuzz_matrix.cpp)
  target_link_libraries(fuzz_matrix PRIVATE bitmap)
  set_target_properties(fuzz_matrix PROPERTIES LINK_FLAGS "${FUZZ_LINK_FLAGS}")
  message(STATUS "Fuzz target fuzz_matrix added.")

  # Fuzzer for BGR to BGRA conversion
  add_executable(fuzz_convert_bgr_to_bgra fuzz/fuzz_convert_bgr_to_bgra.cpp)
  target_link_libraries(fuzz_convert_bgr_to_bgra PRIVATE bitmap)
  set_target_properties(fuzz_convert_bgr_to_bgra PROPERTIES LINK_FLAGS "${FUZZ_LINK_FLAGS}")
  message(STATUS "Fuzz target fuzz_convert_bgr_to_bgra added.")

  # Fuzzer for BGRA to RGBA swizzle
  add_executable(fuzz_swizzle_bgra_to_rgba fuzz/fuzz_swizzle_bgra_to_rgba.cpp)
  target_link_libraries(fuzz_swizzle_bgra_to_rgba PRIVATE bitmap)
  set_target_properties(fuzz_swizzle_bgra_to_rgba PROPERTIES LINK_FLAGS "${FUZZ_LINK_FLAGS}")
  message(STATUS "Fuzz target fuzz_swizzle_bgra_to_rgba added.")

  # Fuzzer for RGBA to BGRA swizzle
  add_executable(fuzz_swizzle_rgba_to_bgra fuzz/fuzz_swizzle_rgba_to_bgra.cpp)
  target_link_libraries(fuzz_swizzle_rgba_to_bgra PRIVATE bitmap)
  set_target_properties(fuzz_swizzle_rgba_to_bgra PROPERTIES LINK_FLAGS "${FUZZ_LINK_FLAGS}")
  message(STATUS "Fuzz target fuzz_swizzle_rgba_to_bgra added.")
  
endif()
# Note: The public include directories from the 'bitmap' target 
# (configured in the main CMakeLists.txt) should be automatically inherited 
# by these fuzz targets because they link against 'bitmap'. 
# This means include paths like "bitmap.hpp" (for top-level include files) 
# or "bitmapfile/bitmap_file.h" (for files within src/) should work 
# in the fuzzers if the fuzzers are updated to use them. 
# The current fuzzers use relative paths like "../../include/bitmap.hpp", 
# which also work due to the file structure but are less clean.
# This is a potential future cleanup for the fuzzer source files, not this CMake update.
