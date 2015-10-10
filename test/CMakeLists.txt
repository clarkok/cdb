find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_subdirectory(utils)
add_subdirectory(driver)

add_executable(test-main test-main.cpp ${UTILS_TEST_SRCS} ${DRIVER_TEST_SRCS})
target_link_libraries(test-main gtest pthread utils driver)
add_test(MainTest test-main)

set (CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake-modules)
if (CMAKE_COMPILER_IS_GNUCXX)
    include(CodeCoverage)
    setup_target_for_coverage(coverage test-main coverage)
endif()
