cmake_minimum_required(VERSION 3.1)
project(raftpb C CXX)

add_definitions(-std=c++11)

aux_source_directory(${PROJECT_SOURCE_DIR} DIR_SRCS)

add_library(raftpb ${DIR_SRCS})

target_link_libraries(raftpb -pthread)
target_link_libraries(raftpb -lprotobuf)
target_link_libraries(raftpb -std=c++11)
