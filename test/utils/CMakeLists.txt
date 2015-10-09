add_executable(buffer-test buffer-test.cpp)
target_link_libraries(buffer-test gtest gtest_main pthread utils)
add_test(BufferTest buffer-test)

add_executable(slice-test slice-test.cpp)
target_link_libraries(slice-test gtest gtest_main pthread utils)
add_test(SliceTest slice-test)
