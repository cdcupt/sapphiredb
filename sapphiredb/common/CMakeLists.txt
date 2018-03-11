cmake_minimum_required(VERSION 2.8.10)
project(common C CXX)

add_definitions(-std=c++11)

aux_source_directory(${PROJECT_SOURCE_DIR} DIR_SRCS)

include_directories(${PROJECT_SOURCE_DIR}/..)

add_subdirectory(PikaLads)

add_library(common ${DIR_SRCS})

target_link_libraries(common PikaLads)