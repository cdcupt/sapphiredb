# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/Cellar/cmake/3.10.2/bin/cmake

# The command to remove a file.
RM = /usr/local/Cellar/cmake/3.10.2/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/cdcupt/cdcupt/sapphiredb/sapphiredb

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/cdcupt/cdcupt/sapphiredb/sapphiredb/build

# Utility rule file for ContinuousUpdate.

# Include the progress variables for this target.
include raft/spdlog/spdlog/CMakeFiles/ContinuousUpdate.dir/progress.make

raft/spdlog/spdlog/CMakeFiles/ContinuousUpdate:
	cd /Users/cdcupt/cdcupt/sapphiredb/sapphiredb/build/raft/spdlog/spdlog && /usr/local/Cellar/cmake/3.10.2/bin/ctest -D ContinuousUpdate

ContinuousUpdate: raft/spdlog/spdlog/CMakeFiles/ContinuousUpdate
ContinuousUpdate: raft/spdlog/spdlog/CMakeFiles/ContinuousUpdate.dir/build.make

.PHONY : ContinuousUpdate

# Rule to build all files generated by this target.
raft/spdlog/spdlog/CMakeFiles/ContinuousUpdate.dir/build: ContinuousUpdate

.PHONY : raft/spdlog/spdlog/CMakeFiles/ContinuousUpdate.dir/build

raft/spdlog/spdlog/CMakeFiles/ContinuousUpdate.dir/clean:
	cd /Users/cdcupt/cdcupt/sapphiredb/sapphiredb/build/raft/spdlog/spdlog && $(CMAKE_COMMAND) -P CMakeFiles/ContinuousUpdate.dir/cmake_clean.cmake
.PHONY : raft/spdlog/spdlog/CMakeFiles/ContinuousUpdate.dir/clean

raft/spdlog/spdlog/CMakeFiles/ContinuousUpdate.dir/depend:
	cd /Users/cdcupt/cdcupt/sapphiredb/sapphiredb/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/cdcupt/cdcupt/sapphiredb/sapphiredb /Users/cdcupt/cdcupt/sapphiredb/sapphiredb/common/spdlog /Users/cdcupt/cdcupt/sapphiredb/sapphiredb/build /Users/cdcupt/cdcupt/sapphiredb/sapphiredb/build/raft/spdlog/spdlog /Users/cdcupt/cdcupt/sapphiredb/sapphiredb/build/raft/spdlog/spdlog/CMakeFiles/ContinuousUpdate.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : raft/spdlog/spdlog/CMakeFiles/ContinuousUpdate.dir/depend

