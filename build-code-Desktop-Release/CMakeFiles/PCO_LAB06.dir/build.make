# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.29

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
CMAKE_COMMAND = /home/ali/.local/lib/python3.10/site-packages/cmake/data/bin/cmake

# The command to remove a file.
RM = /home/ali/.local/lib/python3.10/site-packages/cmake/data/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/ali/pco/pco_lab06/code

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/ali/pco/pco_lab06/build-code-Desktop-Release

# Include any dependencies generated for this target.
include CMakeFiles/PCO_LAB06.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/PCO_LAB06.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/PCO_LAB06.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/PCO_LAB06.dir/flags.make

CMakeFiles/PCO_LAB06.dir/tst_threadpool.cpp.o: CMakeFiles/PCO_LAB06.dir/flags.make
CMakeFiles/PCO_LAB06.dir/tst_threadpool.cpp.o: /home/ali/pco/pco_lab06/code/tst_threadpool.cpp
CMakeFiles/PCO_LAB06.dir/tst_threadpool.cpp.o: CMakeFiles/PCO_LAB06.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/ali/pco/pco_lab06/build-code-Desktop-Release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/PCO_LAB06.dir/tst_threadpool.cpp.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/PCO_LAB06.dir/tst_threadpool.cpp.o -MF CMakeFiles/PCO_LAB06.dir/tst_threadpool.cpp.o.d -o CMakeFiles/PCO_LAB06.dir/tst_threadpool.cpp.o -c /home/ali/pco/pco_lab06/code/tst_threadpool.cpp

CMakeFiles/PCO_LAB06.dir/tst_threadpool.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/PCO_LAB06.dir/tst_threadpool.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ali/pco/pco_lab06/code/tst_threadpool.cpp > CMakeFiles/PCO_LAB06.dir/tst_threadpool.cpp.i

CMakeFiles/PCO_LAB06.dir/tst_threadpool.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/PCO_LAB06.dir/tst_threadpool.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ali/pco/pco_lab06/code/tst_threadpool.cpp -o CMakeFiles/PCO_LAB06.dir/tst_threadpool.cpp.s

# Object files for target PCO_LAB06
PCO_LAB06_OBJECTS = \
"CMakeFiles/PCO_LAB06.dir/tst_threadpool.cpp.o"

# External object files for target PCO_LAB06
PCO_LAB06_EXTERNAL_OBJECTS =

PCO_LAB06: CMakeFiles/PCO_LAB06.dir/tst_threadpool.cpp.o
PCO_LAB06: CMakeFiles/PCO_LAB06.dir/build.make
PCO_LAB06: CMakeFiles/PCO_LAB06.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/ali/pco/pco_lab06/build-code-Desktop-Release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable PCO_LAB06"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/PCO_LAB06.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/PCO_LAB06.dir/build: PCO_LAB06
.PHONY : CMakeFiles/PCO_LAB06.dir/build

CMakeFiles/PCO_LAB06.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/PCO_LAB06.dir/cmake_clean.cmake
.PHONY : CMakeFiles/PCO_LAB06.dir/clean

CMakeFiles/PCO_LAB06.dir/depend:
	cd /home/ali/pco/pco_lab06/build-code-Desktop-Release && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ali/pco/pco_lab06/code /home/ali/pco/pco_lab06/code /home/ali/pco/pco_lab06/build-code-Desktop-Release /home/ali/pco/pco_lab06/build-code-Desktop-Release /home/ali/pco/pco_lab06/build-code-Desktop-Release/CMakeFiles/PCO_LAB06.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/PCO_LAB06.dir/depend

