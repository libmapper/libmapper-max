cmake_minimum_required(VERSION 3.19)

set(BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build")
set(LIBMAPPER_DIR "${BUILD_DIR}/libmapper/libmapper-2.3-dev-win64")
set(MAX_SDK_DIR "${BUILD_DIR}/max-sdk/max-sdk/source/max-sdk-base")
set(C74_SUPPORT_DIR "${MAX_SDK_DIR}/c74support")

project(mapper-max-pd-externals)

set(Liblo_LIB ${LIBMAPPER_DIR}/liblo.lib)
set(Libmapper_LIB ${LIBMAPPER_DIR}/libmapper.lib)
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