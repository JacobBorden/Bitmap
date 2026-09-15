cmake_minimum_required(VERSION 3.16...3.30)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

project(bitmap VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)
enable_testing()
include(FetchContent)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/matrix/CMakeLists.txt")
    add_subdirectory(dependencies/matrix)
else()
    FetchContent_Declare(
        matrix
        GIT_REPOSITORY https://github.com/JacobBorden/Matrix.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(matrix)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/bitmapfile/CMakeLists.txt")
    add_subdirectory(dependencies/bitmapfile)
else()
    FetchContent_Declare(
        bitmapfile
        GIT_REPOSITORY https://github.com/JacobBorden/BitmapFile.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(bitmapfile)
endif()

add_library(bitmap STATIC src/bitmap.cpp)
target_include_directories(bitmap PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/matrix
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/bitmapfile/src
)
target_link_libraries(bitmap PUBLIC matrix bitmapfile)

add_executable(testexe main.cpp)
target_link_libraries(testexe PRIVATE bitmap)

# GoogleTest
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)
add_executable(bitmap_tests tests/test_bitmap.cpp)
target_link_libraries(bitmap_tests PRIVATE bitmap GTest::gtest_main)
gtest_discover_tests(bitmap_tests)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)