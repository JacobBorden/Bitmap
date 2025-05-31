# Fuzzing
include(Fuzzing) # Requires Fuzzing.cmake from https://github.com/google/oss-fuzz/tree/master/infra/base-images/base-builder/Fuzzing.cmake
# Or use your project's specific fuzzing setup.
# This example assumes a Fuzzing_add_fuzzer macro or similar.

# If Fuzzing_add_fuzzer is not available, you might need to define targets manually:
# add_executable(fuzz_bitmap fuzz_bitmap.cpp)
# target_link_libraries(fuzz_bitmap PRIVATE ${OSS_FUZZ_STDLIBS} bitmap) # Assuming 'bitmap' is your main library target

# List of fuzz targets
set(FUZZ_TARGETS
    fuzz_bitmap
    fuzz_bitmap_file
    fuzz_bmp_tool_save
    fuzz_convert_bgr_to_bgra
    fuzz_image_operations
    fuzz_matrix
    fuzz_swizzle_bgra_to_rgba
    fuzz_swizzle_rgba_to_bgra
    fuzz_bmptool_api_manipulations # New fuzz target
)

foreach(TARGET ${FUZZ_TARGETS})
    Fuzzing_add_fuzzer(${TARGET} ${TARGET}.cpp)
    target_link_libraries(${TARGET} PRIVATE bitmap) # Link against your main library
endforeach()
