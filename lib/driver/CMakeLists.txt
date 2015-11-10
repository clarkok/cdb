add_library(driver STATIC driver.cpp driver.hpp basic-driver.cpp basic-driver.hpp block-allocator.cpp
        block-allocator.hpp bitmap-allocator.cpp bitmap-allocator.hpp driver-accesser.cpp driver-accesser.hpp
        basic-accesser.cpp basic-accesser.hpp cached-accesser.hpp cached-accesser.cpp)
target_link_libraries(driver utils)
