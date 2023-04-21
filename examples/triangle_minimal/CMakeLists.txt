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
  add_library(triangle_minimal main.cpp)
  add_compile_definitions(PLATFORM_ANDROID=1)
  include_directories(${CMAKE_ANDROID_NDK}/sources/android/native_app_glue)
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

if(NOT DEFINED NO_SHADERS)
  add_custom_target(shaders DEPENDS ${SHADERS_LIST})
  add_dependencies(triangle_minimal shaders)
  install(DIRECTORY ${CMAKE_BINARY_DIR}/shaders DESTINATION share)
endif()

install(TARGETS triangle_minimal DESTINATION bin)

# android_create_apk(
#     androidcmakeexample
#     "org.triangle_minimal" 
#     "${CMAKE_BINARY_DIR}/apk"
#     "${CMAKE_SOURCE_DIR}/libs"
#     "${CMAKE_SOURCE_DIR}/android"
#     "${CMAKE_SOURCE_DIR}/assets")
