cmake_minimum_required(VERSION 3.19)

include(${MAX_SDK_DIR}/script/max-pretarget.cmake)

message("Generating: ${PROJECT_NAME}")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../build")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Debug")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Release")

add_definitions(
    -D_WINSOCK_DEPRECATED_NO_WARNINGS
    -DHAVE_WINSOCK2_H
    -DNODEFAULTLIB
    -DMAXMSP
)

#############################################################
# MAX EXTERNAL
#############################################################

include_directories( 
    "${MAX_SDK_INCLUDES}"
    "${MAX_SDK_MSP_INCLUDES}"
    "${LIBLO_INCLUDES}"
    "${LIBMAPPER_INCLUDES}"
)

file(GLOB PROJECT_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
add_library( 
    ${PROJECT_NAME}
    MODULE
    ${PROJECT_SRC}
)

include(${MAX_SDK_DIR}/script/max-posttarget.cmake)

target_link_libraries(${PROJECT_NAME} PUBLIC ${Liblo_LIB})
target_link_libraries(${PROJECT_NAME} PUBLIC ${Libmapper_LIB})

if (CMAKE_GENERATOR MATCHES "Visual Studio")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:MSVCRTD")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/INCREMENTAL:NO")
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cmake-pd)