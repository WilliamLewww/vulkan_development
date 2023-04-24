cmake_minimum_required (VERSION 3.21)
project(triangle_minimal)

find_package(Vulkan REQUIRED)

file(GLOB SHADERS 
  "shader.vert" 
  "shader.frag")

add_compile_definitions(SHARE_PATH="${CMAKE_INSTALL_PREFIX}/share")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  add_executable(triangle_minimal main.cpp)
  add_compile_definitions(PLATFORM_LINUX=1)

  find_package(X11)
  target_link_libraries(triangle_minimal X11)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
  add_library(triangle_minimal SHARED main.cpp)
  target_link_libraries(triangle_minimal log)
  target_link_libraries(triangle_minimal android)
  add_compile_definitions(PLATFORM_ANDROID=1)
  add_compile_definitions(ANDROID_STL=c++_shared)

  include_directories(${CMAKE_ANDROID_NDK}/sources/android/native_app_glue)
  add_library(native_app_glue STATIC
      ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
  set(CMAKE_SHARED_LINKER_FLAGS
      "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

  target_link_libraries(triangle_minimal native_app_glue)
endif()

include_directories(triangle_minimal ${Vulkan_INCLUDE_DIRS})
target_link_libraries(triangle_minimal ${Vulkan_LIBRARIES})

if(DEFINED GLSLANG_VALIDATOR_EXECUTABLE)
  set(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE ${GLSLANG_VALIDATOR_EXECUTABLE})
endif()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders/triangle_minimal)
foreach(SHADER ${SHADERS})
        get_filename_component(SHADER_NAME ${SHADER} NAME)

        add_custom_command(
            OUTPUT shaders/triangle_minimal/${SHADER_NAME}.spv
            COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
            --target-env vulkan1.2
            -o shaders/triangle_minimal/${SHADER_NAME}.spv ${SHADER}
            DEPENDS ${SHADER}
        )
        list(APPEND SHADERS_LIST
            shaders/triangle_minimal/${SHADER_NAME}.spv)
endforeach()

add_custom_target(shaders DEPENDS ${SHADERS_LIST})
add_dependencies(triangle_minimal shaders)

install(DIRECTORY ${CMAKE_BINARY_DIR}/shaders DESTINATION share)
install(TARGETS triangle_minimal DESTINATION bin)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
  add_custom_command(TARGET triangle_minimal POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_SOURCE_DIR}/android" "${CMAKE_BINARY_DIR}/android")

  configure_file("${CMAKE_SOURCE_DIR}/shader.vert"
                 "${CMAKE_BINARY_DIR}/android/src/main/shaders/shader.vert")

  configure_file("${CMAKE_SOURCE_DIR}/shader.frag"
                 "${CMAKE_BINARY_DIR}/android/src/main/shaders/shader.frag")

  add_custom_command(TARGET triangle_minimal POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_BINARY_DIR}/libtriangle_minimal.so"
      "${CMAKE_BINARY_DIR}/android/src/main/jniLibs/${CMAKE_ANDROID_ARCH_ABI}/libmain.so")

  add_custom_command(TARGET triangle_minimal
                     COMMAND gradle build 
                     WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/android")
endif()
