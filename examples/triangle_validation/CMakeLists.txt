cmake_minimum_required (VERSION 3.21)
project(triangle_validation)

find_package(Vulkan REQUIRED)
get_filename_component(VULKAN_LIBRARY_PATH ${Vulkan_LIBRARY} DIRECTORY)
if(NOT EXISTS "${VULKAN_LIBRARY_PATH}/libVkLayer_khronos_validation.so")
  message(FATAL_ERROR "Could not find libVkLayer_khronos_validation.so")
endif()

add_compile_definitions(SHARE_PATH="${CMAKE_INSTALL_PREFIX}/share")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  add_executable(triangle_validation main.cpp)
  add_compile_definitions(PLATFORM_LINUX=1)

  find_package(X11)
  target_link_libraries(triangle_validation X11)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
  add_library(triangle_validation SHARED main.cpp)
  target_link_libraries(triangle_validation log)
  target_link_libraries(triangle_validation android)
  add_compile_definitions(PLATFORM_ANDROID=1)
  add_compile_definitions(ANDROID_STL=c++_shared)

  include_directories(${CMAKE_ANDROID_NDK}/sources/android/native_app_glue)
  add_library(native_app_glue STATIC
      ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
  set(CMAKE_SHARED_LINKER_FLAGS
      "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

  target_link_libraries(triangle_validation native_app_glue)
endif()

include_directories(triangle_validation ${Vulkan_INCLUDE_DIRS})
target_link_libraries(triangle_validation ${Vulkan_LIBRARIES})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
  add_custom_command(TARGET triangle_validation POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_SOURCE_DIR}/android" "${CMAKE_BINARY_DIR}/android")

  configure_file("${CMAKE_SOURCE_DIR}/shader.vert"
                 "${CMAKE_BINARY_DIR}/android/src/main/shaders/shader.vert")

  configure_file("${CMAKE_SOURCE_DIR}/shader.frag"
                 "${CMAKE_BINARY_DIR}/android/src/main/shaders/shader.frag")

  add_custom_command(TARGET triangle_validation POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_BINARY_DIR}/libtriangle_validation.so"
      "${CMAKE_BINARY_DIR}/android/src/main/jniLibs/${CMAKE_ANDROID_ARCH_ABI}/libmain.so")

  add_custom_command(TARGET triangle_validation POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
      "${VULKAN_LIBRARY_PATH}/libVkLayer_khronos_validation.so"
      "${CMAKE_BINARY_DIR}/android/src/main/jniLibs/${CMAKE_ANDROID_ARCH_ABI}/libVkLayer_khronos_validation.so")

  add_custom_command(TARGET triangle_validation
                     COMMAND gradle build 
                     WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/android")

  install(DIRECTORY ${CMAKE_BINARY_DIR}/android/build/outputs/apk DESTINATION ${CMAKE_INSTALL_PREFIX})
else()
  if(DEFINED GLSLANG_VALIDATOR_EXECUTABLE)
    set(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE ${GLSLANG_VALIDATOR_EXECUTABLE})
  endif()
 
  file(GLOB SHADERS 
    "shader.vert" 
    "shader.frag")
 
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders/triangle_validation)
  foreach(SHADER ${SHADERS})
          get_filename_component(SHADER_NAME ${SHADER} NAME)
  
          add_custom_command(
              OUTPUT shaders/triangle_validation/${SHADER_NAME}.spv
              COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
              --target-env vulkan1.2
              -o shaders/triangle_validation/${SHADER_NAME}.spv ${SHADER}
              DEPENDS ${SHADER}
          )
          list(APPEND SHADERS_LIST
              shaders/triangle_validation/${SHADER_NAME}.spv)
  endforeach()
  
  add_custom_target(shaders DEPENDS ${SHADERS_LIST})
  add_dependencies(triangle_validation shaders)
  
  install(DIRECTORY ${CMAKE_BINARY_DIR}/shaders DESTINATION share)
  install(TARGETS triangle_validation DESTINATION bin)
endif()
