# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /mnt/d/mini-sql-master-0614

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /mnt/d/mini-sql-master-0614/cmake-build-debug

# Utility rule file for ExperimentalBuild.

# Include any custom commands dependencies for this target.
include glog-build/CMakeFiles/ExperimentalBuild.dir/compiler_depend.make

# Include the progress variables for this target.
include glog-build/CMakeFiles/ExperimentalBuild.dir/progress.make

glog-build/CMakeFiles/ExperimentalBuild:
	cd /mnt/d/mini-sql-master-0614/cmake-build-debug/glog-build && /usr/bin/ctest -D ExperimentalBuild

ExperimentalBuild: glog-build/CMakeFiles/ExperimentalBuild
ExperimentalBuild: glog-build/CMakeFiles/ExperimentalBuild.dir/build.make
.PHONY : ExperimentalBuild

# Rule to build all files generated by this target.
glog-build/CMakeFiles/ExperimentalBuild.dir/build: ExperimentalBuild
.PHONY : glog-build/CMakeFiles/ExperimentalBuild.dir/build

glog-build/CMakeFiles/ExperimentalBuild.dir/clean:
	cd /mnt/d/mini-sql-master-0614/cmake-build-debug/glog-build && $(CMAKE_COMMAND) -P CMakeFiles/ExperimentalBuild.dir/cmake_clean.cmake
.PHONY : glog-build/CMakeFiles/ExperimentalBuild.dir/clean

glog-build/CMakeFiles/ExperimentalBuild.dir/depend:
	cd /mnt/d/mini-sql-master-0614/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/d/mini-sql-master-0614 /mnt/d/mini-sql-master-0614/thirdparty/glog /mnt/d/mini-sql-master-0614/cmake-build-debug /mnt/d/mini-sql-master-0614/cmake-build-debug/glog-build /mnt/d/mini-sql-master-0614/cmake-build-debug/glog-build/CMakeFiles/ExperimentalBuild.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : glog-build/CMakeFiles/ExperimentalBuild.dir/depend

