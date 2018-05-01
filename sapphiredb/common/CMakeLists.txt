cmake_minimum_required(VERSION 3.1)
project(common C CXX)

#SET(CMAKE_BUILD_TYPE "Debug")  
#SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")  
#SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall") 

add_definitions(-std=c++11)

aux_source_directory(${PROJECT_SOURCE_DIR} DIR_SRCS)

include_directories(${PROJECT_SOURCE_DIR}/..)

add_subdirectory(spdlog)

IF (LINUX)
    add_library(common epoll.cc net.cc timer.cc uniqueid.cc)
ELSEIF (UNIX)
    add_library(common kqueue.cc net.cc timer.cc uniqueid.cc)
ENDIF ()

target_link_libraries(common spdlog)
target_link_libraries(common -pthread)
target_link_libraries(common -std=c++11)
