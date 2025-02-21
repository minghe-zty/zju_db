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

# Include any dependencies generated for this target.
include test/CMakeFiles/index_iterator_test.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include test/CMakeFiles/index_iterator_test.dir/compiler_depend.make

# Include the progress variables for this target.
include test/CMakeFiles/index_iterator_test.dir/progress.make

# Include the compile flags for this target's objects.
include test/CMakeFiles/index_iterator_test.dir/flags.make

test/CMakeFiles/index_iterator_test.dir/index/index_iterator_test.cpp.o: test/CMakeFiles/index_iterator_test.dir/flags.make
test/CMakeFiles/index_iterator_test.dir/index/index_iterator_test.cpp.o: ../test/index/index_iterator_test.cpp
test/CMakeFiles/index_iterator_test.dir/index/index_iterator_test.cpp.o: test/CMakeFiles/index_iterator_test.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/d/mini-sql-master-0614/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object test/CMakeFiles/index_iterator_test.dir/index/index_iterator_test.cpp.o"
	cd /mnt/d/mini-sql-master-0614/cmake-build-debug/test && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT test/CMakeFiles/index_iterator_test.dir/index/index_iterator_test.cpp.o -MF CMakeFiles/index_iterator_test.dir/index/index_iterator_test.cpp.o.d -o CMakeFiles/index_iterator_test.dir/index/index_iterator_test.cpp.o -c /mnt/d/mini-sql-master-0614/test/index/index_iterator_test.cpp

test/CMakeFiles/index_iterator_test.dir/index/index_iterator_test.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/index_iterator_test.dir/index/index_iterator_test.cpp.i"
	cd /mnt/d/mini-sql-master-0614/cmake-build-debug/test && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /mnt/d/mini-sql-master-0614/test/index/index_iterator_test.cpp > CMakeFiles/index_iterator_test.dir/index/index_iterator_test.cpp.i

test/CMakeFiles/index_iterator_test.dir/index/index_iterator_test.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/index_iterator_test.dir/index/index_iterator_test.cpp.s"
	cd /mnt/d/mini-sql-master-0614/cmake-build-debug/test && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /mnt/d/mini-sql-master-0614/test/index/index_iterator_test.cpp -o CMakeFiles/index_iterator_test.dir/index/index_iterator_test.cpp.s

# Object files for target index_iterator_test
index_iterator_test_OBJECTS = \
"CMakeFiles/index_iterator_test.dir/index/index_iterator_test.cpp.o"

# External object files for target index_iterator_test
index_iterator_test_EXTERNAL_OBJECTS =

test/index_iterator_test: test/CMakeFiles/index_iterator_test.dir/index/index_iterator_test.cpp.o
test/index_iterator_test: test/CMakeFiles/index_iterator_test.dir/build.make
test/index_iterator_test: bin/libzSql.so
test/index_iterator_test: test/libminisql_test_main.so
test/index_iterator_test: glog-build/libglogd.so.0.6.0
test/index_iterator_test: lib/libgtestd.so.1.11.0
test/index_iterator_test: test/CMakeFiles/index_iterator_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/mnt/d/mini-sql-master-0614/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable index_iterator_test"
	cd /mnt/d/mini-sql-master-0614/cmake-build-debug/test && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/index_iterator_test.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
test/CMakeFiles/index_iterator_test.dir/build: test/index_iterator_test
.PHONY : test/CMakeFiles/index_iterator_test.dir/build

test/CMakeFiles/index_iterator_test.dir/clean:
	cd /mnt/d/mini-sql-master-0614/cmake-build-debug/test && $(CMAKE_COMMAND) -P CMakeFiles/index_iterator_test.dir/cmake_clean.cmake
.PHONY : test/CMakeFiles/index_iterator_test.dir/clean

test/CMakeFiles/index_iterator_test.dir/depend:
	cd /mnt/d/mini-sql-master-0614/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/d/mini-sql-master-0614 /mnt/d/mini-sql-master-0614/test /mnt/d/mini-sql-master-0614/cmake-build-debug /mnt/d/mini-sql-master-0614/cmake-build-debug/test /mnt/d/mini-sql-master-0614/cmake-build-debug/test/CMakeFiles/index_iterator_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : test/CMakeFiles/index_iterator_test.dir/depend

