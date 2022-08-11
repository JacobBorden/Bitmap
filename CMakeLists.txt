cmake_minimum_required(VERSION 3.0.0)
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
add_library(bitmap STATIC src/bitmap.cpp )
link_libraries(bitmap)
add_executable(testexe main.cpp)



set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set( CPACK_PROJECT_VERSION ${PROJECT_VERSION}) 
include (Cpack)