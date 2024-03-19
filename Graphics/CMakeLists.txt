cmake_minimum_required(VERSION 3.8)

file(GLOB SRCS ./*.h ./*.cpp)

add_library(Graphics STATIC ${SRCS})
add_library(lib::Graphics ALIAS Graphics)

set_target_properties(Graphics PROPERTIES
    LINKER_LANGUAGE CXX
    INCLUDE_DIRECTORIES ${INC_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${LIB_DIR}
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${LIB_DIR}
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${LIB_DIR}
)