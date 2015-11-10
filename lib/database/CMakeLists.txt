add_library(database STATIC database.hpp database.cpp)
target_link_libraries(database table)
