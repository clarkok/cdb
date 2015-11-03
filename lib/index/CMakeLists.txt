add_library(index STATIC btree.cpp btree.hpp btree-intl.hpp linear-table.cpp linear-table-intl.hpp linear-table.hpp
        skip-table.cpp skip-table.hpp)
target_link_libraries(index utils)
