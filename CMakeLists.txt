cmake_minimum_required (VERSION 2.6)
project (cdb)

#set (CMAKE_CXX_COMPILER "clang++-3.6")
#set (CMAKE_COMPILER_IS_GNUCXX false)
set (CMAKE_EXPORT_COMPILE_COMMANDS "ON")

if (CMAKE_COMPILER_IS_GNUCXX)
    set (CMAKE_CXX_FLAGS " -std=c++11 -g -O0 -fprofile-arcs -ftest-coverage -Wall -Wextra ")
else()
    set (CMAKE_CXX_FLAGS " -std=c++11 -g -O3 -Wall -Wextra ")
endif()

enable_testing ()

include_directories (${PROJECT_SOURCE_DIR})

add_subdirectory (lib)
add_subdirectory (src)
add_subdirectory (test)
