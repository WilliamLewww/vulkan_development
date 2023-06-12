#
# CMake Toolchain file for crosscompiling on ARM.
#
# Target operating system name.
set(CMAKE_SYSTEM_NAME Android)
set(CMAKE_SYSTEM_VERSION 33)
set(CMAKE_ANDROID_ARCH_ABI arm64-v8a)
set(CMAKE_ANDROID_NDK "/opt/android_sdk/ndk/25.2.9519653")

get_filename_component(CURRENT_PARENT_PATH ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
set(CMAKE_FIND_ROOT_PATH "${CURRENT_PARENT_PATH}/_out/arm64-v8a-android33")

# Adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment only.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search headers and libraries in the target environment only.
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
