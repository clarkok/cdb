add_library(table STATIC table.cpp table.hpp schema.cpp schema.hpp view.cpp view.hpp index-view.cpp index-view.hpp skip-view.cpp skip-view.hpp)
target_link_libraries(table driver index condition utils)
