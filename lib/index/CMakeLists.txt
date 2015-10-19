add_library(index STATIC btree.cpp linear-table.cpp)
target_link_libraries(index utils)
