cmake_minimum_required (VERSION 2.6)
project (cdb)

set (CMAKE_EXPORT_COMPILE_COMMANDS "ON")

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|Clang")
    set (CMAKE_CXX_FLAGS " -std=c++11 -g -Wall -Wextra")
endif ()

option(Perf "Perf" OFF)
if (Perf)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif ()

option(Coverage "Coverage" OFF)
if (Coverage)
    set (CMAKE_CXX_COMPILER "/usr/bin/g++")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -fprofile-arcs -ftest-coverage")
endif ()

include_directories (${PROJECT_SOURCE_DIR})

add_subdirectory (lib)
add_subdirectory (src)

option(Test "Test" OFF)

if (Test)
    enable_testing ()
    add_subdirectory (test)
endif ()
