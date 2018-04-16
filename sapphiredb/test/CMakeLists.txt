cmake_minimum_required(VERSION 3.1)
project(test C CXX)

add_definitions(-std=c++11)

#add_subdirectory("${PROJECT_SOURCE_DIR}/.." "${CMAKE_CURRENT_BINARY_DIR}/common")
#add_subdirectory("${PROJECT_SOURCE_DIR}/.." "${CMAKE_CURRENT_BINARY_DIR}/raft")
include_directories(../common ../raft)

include_directories(${PROJECT_SOURCE_DIR}/..)

SET(CMAKE_BUILD_TYPE "Debug")  
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")  
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall") 

file( GLOB APP_SOURCES ${PROJECT_SOURCE_DIR}/*.cc )
foreach( testsourcefile ${APP_SOURCES} )
    # I used a simple string replace, to cut off .cpp.
    string( REPLACE "_test.cc" "" testname ${testsourcefile} )
    string( REPLACE "/" "_" testname ${testsourcefile} )
    add_executable( ${testname} ${testsourcefile} )
    # Make sure YourLib is linked to each app
    
    target_link_libraries(${testname} raft)
    target_link_libraries(${testname} common)
    target_link_libraries(${testname} -pthread)
    target_link_libraries(${testname} -std=c++11)
endforeach( testsourcefile ${APP_SOURCES} )
