cmake_minimum_required(VERSION 3.1)
project(common C CXX)

add_definitions(-std=c++11)

aux_source_directory(${PROJECT_SOURCE_DIR} DIR_SRCS)

add_subdirectory(spdlog)

add_library(common ${DIR_SRCS})

target_link_libraries(common spdlog)
