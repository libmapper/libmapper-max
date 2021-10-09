cmake_minimum_required(VERSION 3.19)

# Replace with local paths before building
set(MAX_SDK_DIR "C:/Users/brady/Documents/GitHub/McGill/max-sdk/source/max-sdk-base")
set(BUILD_DIR "C:/Users/brady/Documents/GitHub/McGill/libmapper/build")
set(LIBLO_DIR "C:/Users/brady/Documents/GitHub/McGill/liblo")
set(LIBLO_BUILD_DIR "${LIBLO_DIR}/cmake/build/Debug")
set(LIBLO_INCLUDES "${LIBLO_DIR}/cmake/build;${LIBLO_DIR}")
set(LIBMAPPER_DIR "C:/Users/brady/Documents/GitHub/McGill/libmapper/")
set(LIBMAPPER_BUILD_DIR "${LIBMAPPER_DIR}/build/Debug")
set(C74_SUPPORT_DIR "C:/Users/brady/Documents/GitHub/McGill/max-sdk/source/max-sdk-base/c74support")

project(libmapper-max-externals)

set(Liblo_LIB ${LIBLO_BUILD_DIR}/liblo.lib)
set(Libmapper_LIB ${LIBMAPPER_BUILD_DIR}/libmapper.lib)
mark_as_advanced(Liblo_LIB)
mark_as_advanced(Libmapper_LIB)

# Add subdirectories for each external
set(EXTERNALS_LIST "${CMAKE_CURRENT_SOURCE_DIR}/mapper;${CMAKE_CURRENT_SOURCE_DIR}/mpr_device;${CMAKE_CURRENT_SOURCE_DIR}/mpr_in;${CMAKE_CURRENT_SOURCE_DIR}/mpr_out")

foreach (external_dir ${EXTERNALS_LIST})
  #message("checking in: ${external_dir}")
  if (EXISTS "${external_dir}/CMakeLists.txt")
    add_subdirectory(${external_dir})
  endif ()
endforeach ()