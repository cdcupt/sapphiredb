cmake_minimum_required(VERSION 2.8.10)
project(sapphire C CXX)

aux_source_directory(${PROJECT_SOURCE_DIR} DIR_SRCS)

add_subdirectory(raft)
add_subdirectory(common)

add_executable(sapphiredb main.cc)


target_link_libraries(sapphiredb raft)
target_link_libraries(sapphiredb common)