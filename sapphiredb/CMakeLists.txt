cmake_minimum_required(VERSION 3.1)
project(sapphiredb C CXX)

add_definitions(-std=c++11)

aux_source_directory(${PROJECT_SOURCE_DIR} DIR_SRCS)

include_directories(${PROJECT_SOURCE_DIR}/..)

IF (APPLE)
    option(UNIX_LIKE "UNIX_LIKE platform" ON)
    configure_file (${PROJECT_SOURCE_DIR}/include/config_unix.h.in ${PROJECT_SOURCE_DIR}/include/config.h)
ELSE ()
    option(LINUX "LINUX platfrom" ON)
    configure_file (${PROJECT_SOURCE_DIR}/include/config_linux.h.in ${PROJECT_SOURCE_DIR}/include/config.h)
ENDIF ()

add_subdirectory(raft raft)
add_subdirectory(common common)
add_subdirectory(test test)

#add_executable(sapphiredb main.cc)

#target_link_libraries(sapphiredb raft)
#target_link_libraries(sapphiredb common)
#target_link_libraries(sapphiredb test)
#target_link_libraries(sapphiredb -pthread)
#target_link_libraries(sapphiredb -std=c++11)