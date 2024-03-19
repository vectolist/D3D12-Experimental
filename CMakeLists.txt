#Root CMake
cmake_minimum_required(VERSION 3.8)
#supress zero check
set(CMAKE_SUPPRESS_REGENERATION true)
#main project
project(D3D12-Experimental)

#globar scope
set(INC_DIR ${CMAKE_SOURCE_DIR}/Graphics)
set(LIB_DIR ${CMAKE_SOURCE_DIR}/lib)
set(BIN_DIR ${CMAKE_SOURCE_DIR}/bin)

set(LIBS Graphics.lib)

add_subdirectory(Graphics)                      #lib
add_subdirectory(Sample/Framework)