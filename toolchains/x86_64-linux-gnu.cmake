#
# CMake Toolchain file for crosscompiling on x86.
#
# Target operating system name.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_CROSSCOMPILING TRUE)

# Name of C compiler.
set(CMAKE_C_COMPILER "/usr/bin/x86_64-linux-gnu-gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/x86_64-linux-gnu-g++")

# Where to look for the target environment. (More paths can be added here)
set(CMAKE_FIND_ROOT_PATH /usr/bin/x86_64-linux-gnu)
set(CMAKE_INCLUDE_PATH  /usr/bin/x86_64-linux-gnu/include)
set(CMAKE_LIBRARY_PATH  /usr/bin/x86_64-linux-gnu/lib)
set(CMAKE_PROGRAM_PATH  /usr/bin/x86_64-linux-gnu/bin)

get_filename_component(CURRENT_PARENT_PATH ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
set(CMAKE_FIND_ROOT_PATH "${CURRENT_PARENT_PATH}/_out/x86_64-linux-gnu")

# Adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment only.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search headers and libraries in the target environment only.
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
