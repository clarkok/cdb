# add_executable(basic-driver-test basic-driver-test.cpp)
# target_link_libraries(basic-driver-test gtest gtest_main pthread driver)
# add_test(BasicDriverTest basic-driver-test)
# 
# add_executable(bitmap-allocator-test bitmap-allocator-test.cpp)
# target_link_libraries(bitmap-allocator-test gtest gtest_main pthread driver)
# add_test(BitmapAllocatorTest bitmap-allocator-test)

set(DRIVER_TEST_SRCS 
    ${CMAKE_CURRENT_SOURCE_DIR}/basic-driver-test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bitmap-allocator-test.cpp
    PARENT_SCOPE)
