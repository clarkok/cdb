if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|Clang")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-undefined-inline -Wno-unused-const-variable -Wno-sign-compare")
endif ()

set (CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/third-party)
if (Coverage)
    include(cmake-modules/CodeCoverage)
    setup_target_for_coverage(coverage test-main coverage)
endif()

find_package(Threads REQUIRED)

include_directories(${gtest_SOURCE_DIR}/include)
add_subdirectory(utils)
add_subdirectory(driver)
add_subdirectory(index)
add_subdirectory(parser)

find_library(profiler profiler /usr/lib)

add_executable(test-main test-main.cpp 
    ${UTILS_TEST_SRCS} 
    ${DRIVER_TEST_SRCS}
    ${INDEX_TEST_SRCS}
    ${PARSER_TEST_SRCS}
    )

if ("${CMAKE_CXX_COMPILER_ID}" EQUAL "MSVC")
    target_link_libraries(test-main gtest utils driver index)
else ()
    target_link_libraries(test-main gtest pthread utils driver index)
endif ()
add_test(MainTest test-main)
