cmake_minimum_required(VERSION 3.19)

set(BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build")

if (CMAKE_GENERATOR MATCHES "Visual Studio")
    set(LIBMAPPER_DIR "${BUILD_DIR}/libmapper/libmapper-main")
    set(LIBMAPPER_BUILD_DIR "${LIBMAPPER_DIR}/build/Release")
    set(LIBLO_DIR "${LIBMAPPER_DIR}/build/liblo/liblo-master")
    set(LIBLO_BUILD_DIR "${LIBLO_DIR}/cmake/build/Release")
    set(LIBLO_INCLUDES "${LIBLO_DIR}/cmake/build;${LIBLO_DIR}")
    set(Liblo_LIB ${LIBLO_BUILD_DIR}/liblo.lib)
    set(Libmapper_LIB ${LIBMAPPER_BUILD_DIR}/libmapper.lib)
    set(LIBMAPPER_INCLUDES ${LIBMAPPER_DIR}/include)
else()
    set(LIBMAPPER_INCLUDES "${BUILD_DIR}/inst/include")
    set(LIBLO_INCLUDES "${BUILD_DIR}/inst/include")
    set(Liblo_LIB ${BUILD_DIR}/inst/lib/liblo.dylib)
    set(Libmapper_LIB ${BUILD_DIR}/inst/lib/libmapper.dylib)
endif()

set(MAX_SDK_DIR "${BUILD_DIR}/max-sdk/source/max-sdk-base")
set(C74_SUPPORT_DIR "${MAX_SDK_DIR}/c74support")

project(mapper-max-pd-externals)

mark_as_advanced(Liblo_LIB)
mark_as_advanced(Libmapper_LIB)

if (APPLE)
    set(CMAKE_OSX_ARCHITECTURES x86_64;arm64)
endif()

# Add subdirectories for each external
set(EXTERNALS_LIST "${CMAKE_CURRENT_SOURCE_DIR}/mapper;${CMAKE_CURRENT_SOURCE_DIR}/mpr.device;${CMAKE_CURRENT_SOURCE_DIR}/mpr.in;${CMAKE_CURRENT_SOURCE_DIR}/mpr.out;${CMAKE_CURRENT_SOURCE_DIR}/oscmulticast")

foreach (external_dir ${EXTERNALS_LIST})
  #message("checking in: ${external_dir}")
  if (EXISTS "${external_dir}/CMakeLists.txt")
    add_subdirectory(${external_dir})
  endif ()
endforeach ()