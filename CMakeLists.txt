cmake_minimum_required (VERSION 2.6)
project (cdb)

set (CMAKE_EXPORT_COMPILE_COMMANDS "ON")

if (APPLE)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|Clang")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall -Wextra")
endif ()

if (CYGWIN)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
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

option(Test "Test" ON)
if (Test)
    enable_testing ()
    include_directories(${gtest_SOURCE_DIR}/include)
    add_definitions(-DDB_TEST)
    add_subdirectory (third-party/gtest-1.7.0)
    add_subdirectory (test)
endif ()

add_subdirectory (lib)
add_subdirectory (src)
