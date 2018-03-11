cmake_minimum_required(VERSION 2.8.10)
project(PikaLads C CXX)

add_definitions(-std=c++11)

aux_source_directory(${PROJECT_SOURCE_DIR} DIR_SRCS)

add_subdirectory(${PROJECT_SOURCE_DIR}/slash/slash/src)

add_library(PikaLads ${DIR_SRCS})

target_link_libraries(PikaLads slash)