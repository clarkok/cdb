set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-undefined-inline -Wno-unused-const-variable")
if (CMAKE_COMPILER_IS_GNUCXX)
    set (CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake-modules)
    include(CodeCoverage)
    setup_target_for_coverage(coverage test-main coverage)
endif()

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_subdirectory(utils)
add_subdirectory(driver)
add_subdirectory(index)

find_library(profiler profiler /usr/lib)

add_executable(test-main test-main.cpp ${UTILS_TEST_SRCS} ${DRIVER_TEST_SRCS} ${INDEX_TEST_SRCS})
target_link_libraries(test-main gtest pthread utils driver index profiler)
add_test(MainTest test-main)
