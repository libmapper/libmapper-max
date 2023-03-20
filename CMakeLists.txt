cmake_minimum_required(VERSION 3.19)

set(BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build")
set(LIBMAPPER_DIR "${BUILD_DIR}/libmapper/libmapper-main")
set(LIBMAPPER_BUILD_DIR "${LIBMAPPER_DIR}/build/Release")
set(LIBLO_DIR "${LIBMAPPER_DIR}/build/liblo/liblo-master")
set(LIBLO_BUILD_DIR "${LIBLO_DIR}/cmake/build/Release")
set(LIBLO_INCLUDES "${LIBLO_DIR}/cmake/build;${LIBLO_DIR}")
set(MAX_SDK_DIR "${BUILD_DIR}/max-sdk/max-sdk/source/max-sdk-base")
set(C74_SUPPORT_DIR "${MAX_SDK_DIR}/c74support")

project(mapper-max-pd-externals)

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