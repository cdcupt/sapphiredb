cmake_minimum_required(VERSION 3.1)
project(raft C CXX)

SET(CMAKE_BUILD_TYPE "Debug")  
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")  
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall") 

add_definitions(-std=c++11)

aux_source_directory(${PROJECT_SOURCE_DIR} DIR_SRCS)

include_directories(${PROJECT_SOURCE_DIR}/..)
#include_directories(../common)

add_subdirectory(raftpb)
#add_subdirectory("${PROJECT_SOURCE_DIR}/.." "${CMAKE_CURRENT_BINARY_DIR}/common")

add_library(raft ${DIR_SRCS})

target_link_libraries(raft raftpb)
#target_link_libraries(raft common)
target_link_libraries(raft -pthread)
target_link_libraries(raft -lprotobuf)