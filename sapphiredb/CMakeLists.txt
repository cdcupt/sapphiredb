cmake_minimum_required(VERSION 3.1)
project(sapphiredb C CXX)

add_definitions(-std=c++11)

aux_source_directory(${PROJECT_SOURCE_DIR} DIR_SRCS)

add_subdirectory(raft)

add_executable(sapphiredb main.cc)

target_link_libraries(sapphiredb raft)
target_link_libraries(raftpb -pthread)
target_link_libraries(raftpb -std=c++11)