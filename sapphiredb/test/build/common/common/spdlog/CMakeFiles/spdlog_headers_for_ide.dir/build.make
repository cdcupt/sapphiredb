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
CMAKE_SOURCE_DIR = /Users/cdcupt/cdcupt/sapphiredb/sapphiredb/test

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/cdcupt/cdcupt/sapphiredb/sapphiredb/test/build

# Utility rule file for spdlog_headers_for_ide.

# Include the progress variables for this target.
include common/common/spdlog/CMakeFiles/spdlog_headers_for_ide.dir/progress.make

spdlog_headers_for_ide: common/common/spdlog/CMakeFiles/spdlog_headers_for_ide.dir/build.make

.PHONY : spdlog_headers_for_ide

# Rule to build all files generated by this target.
common/common/spdlog/CMakeFiles/spdlog_headers_for_ide.dir/build: spdlog_headers_for_ide

.PHONY : common/common/spdlog/CMakeFiles/spdlog_headers_for_ide.dir/build

common/common/spdlog/CMakeFiles/spdlog_headers_for_ide.dir/clean:
	cd /Users/cdcupt/cdcupt/sapphiredb/sapphiredb/test/build/common/common/spdlog && $(CMAKE_COMMAND) -P CMakeFiles/spdlog_headers_for_ide.dir/cmake_clean.cmake
.PHONY : common/common/spdlog/CMakeFiles/spdlog_headers_for_ide.dir/clean

common/common/spdlog/CMakeFiles/spdlog_headers_for_ide.dir/depend:
	cd /Users/cdcupt/cdcupt/sapphiredb/sapphiredb/test/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/cdcupt/cdcupt/sapphiredb/sapphiredb/test /Users/cdcupt/cdcupt/sapphiredb/sapphiredb/common/spdlog /Users/cdcupt/cdcupt/sapphiredb/sapphiredb/test/build /Users/cdcupt/cdcupt/sapphiredb/sapphiredb/test/build/common/common/spdlog /Users/cdcupt/cdcupt/sapphiredb/sapphiredb/test/build/common/common/spdlog/CMakeFiles/spdlog_headers_for_ide.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : common/common/spdlog/CMakeFiles/spdlog_headers_for_ide.dir/depend

