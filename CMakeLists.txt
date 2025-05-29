cmake_minimum_required(VERSION 3.10)
project(bitmap VERSION 1.0.0)

option(ENABLE_FUZZING "Enable fuzzing builds" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

if(ENABLE_FUZZING)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    find_program(CLANG_COMPILER NAMES clang++ clang)
    if(CLANG_COMPILER)
      set(CMAKE_CXX_COMPILER ${CLANG_COMPILER})
    else()
      message(WARNING "Clang compiler not found. Disabling fuzzing.")
      set(ENABLE_FUZZING OFF)
    endif()
  endif()

  if(ENABLE_FUZZING AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Fuzzing enabled. Using Clang compiler.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=address,fuzzer-no-link")
    # For linking fuzz targets, we'll use -fsanitize=fuzzer later
  else()
    message(STATUS "Fuzzing disabled or Clang not used.")
  endif()
endif()

include(CTest)
enable_testing()

# Add your main library with all sources
add_library(bitmap STATIC 
    src/bitmap/bitmap.cpp
    src/bitmapfile/bitmap_file.cpp
    src/format/bitmap.cpp
)

# Executable for main.cpp (if it's a demo or separate utility)
add_executable(testexe main.cpp)
target_link_libraries(testexe PRIVATE bitmap)

target_include_directories(bitmap PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
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
