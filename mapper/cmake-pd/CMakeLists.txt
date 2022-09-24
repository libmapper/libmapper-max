cmake_minimum_required(VERSION 3.19)

message("Generating: ${PROJECT_NAME}")

#############################################################
# Pure Data EXTERNAL
#############################################################

# Include pd.cmake:  
include(${CMAKE_CURRENT_SOURCE_DIR}/pd.cmake)

# Define the path to the Pure Data sources:
set_pd_sources(${BUILD_DIR}/pure-data/pure-data-0.52-2/src)

# Set the output path for the external:  
set_pd_external_path(${BUILD_DIR}/mapper/Debug/)

# Add the mapper external:
file(GLOB MAPPER_SRCS
  "${CMAKE_CURRENT_SOURCE_DIR}/../mapper.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../mapper_pd.def"
)
add_pd_external(mapper-pd-project mapper "${MAPPER_SRCS}")

add_definitions(
    -D_WINSOCK_DEPRECATED_NO_WARNINGS
    -DHAVE_WINSOCK2_H
    -DNODEFAULTLIB
)

remove_definitions(
    -DMAXMSP
)

include_directories( 
  "${LIBLO_INCLUDES}"
  "${LIBMAPPER_DIR}/include"
)

target_link_libraries(mapper-pd-project PUBLIC ${Liblo_LIB})
target_link_libraries(mapper-pd-project PUBLIC ${Libmapper_LIB})

set_target_properties(mapper-pd-project PROPERTIES LINK_FLAGS "/NODEFAULTLIB:MSVCRTD")