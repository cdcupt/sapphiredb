cmake_minimum_required(VERSION 2.8.10)
project(raft C CXX)

add_definitions(-std=c++11)

aux_source_directory(${PROJECT_SOURCE_DIR} DIR_SRCS)

include_directories(${PROJECT_SOURCE_DIR}/..)

add_subdirectory(raftpb)
add_subdirectory("${PROJECT_SOURCE_DIR}/../common" "${CMAKE_CURRENT_BINARY_DIR}/testlib_build")

add_library(raft ${DIR_SRCS})

target_link_libraries(raft raftpb)
target_link_libraries(raft common)
target_link_libraries(raft -pthread)
target_link_libraries(raft -lprotobuf)