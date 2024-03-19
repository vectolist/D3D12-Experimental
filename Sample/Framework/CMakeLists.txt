cmake_minimum_required(VERSION 3.8)


add_executable(ConstBuffer main.cpp)

add_dependencies(ConstBuffer lib::Graphics)

set_target_properties(ConstBuffer PROPERTIES
    CXX_STANDARD    17
    LINKER_LANGUAGE "CXX"
    INCLUDE_DIRECTORIES ${INC_DIR}
    LINK_DIRECTORIES ${LIB_DIR}
    LINK_LIBRARIES ${LIBS}
    RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BIN_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BIN_DIR}
)