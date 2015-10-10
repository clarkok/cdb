cmake_minimum_required (VERSION 2.6)
project (cdb)

set (CMAKE_CXX_COMPILER "clang++-3.6")
set (CMAKE_CXX_FLAGS "-std=c++11 -g -O0 -fprofile-arcs -ftest-coverage -Wall -Wextra")
set (CMAKE_EXPORT_COMPILE_COMMANDS "ON")

enable_testing ()

include_directories (${PROJECT_SOURCE_DIR})

add_subdirectory (lib)
add_subdirectory (src)
add_subdirectory (test)
