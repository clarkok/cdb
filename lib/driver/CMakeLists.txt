add_library(driver STATIC driver.cpp basic-driver.cpp block-allocator.cpp bitmap-allocator.cpp driver-accesser.cpp
    basic-accesser.cpp)
target_link_libraries(driver utils)
