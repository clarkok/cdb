add_executable(db main.cpp)
target_link_libraries(db condition database driver index parser table utils)
