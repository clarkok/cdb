# add_executable(buffer-test buffer-test.cpp)
# target_link_libraries(buffer-test gtest gtest_main pthread utils)
# add_test(BufferTest buffer-test)
# 
# add_executable(slice-test slice-test.cpp)
# target_link_libraries(slice-test gtest gtest_main pthread utils)
# add_test(SliceTest slice-test)

set(UTILS_TEST_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer-test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/slice-test.cpp
    PARENT_SCOPE)
