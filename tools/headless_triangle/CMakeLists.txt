cmake_minimum_required (VERSION 3.21)
project(headless_triangle)

find_package(Vulkan REQUIRED)

file(GLOB SHADERS 
  "shader.vert" 
  "shader.frag")

add_executable(application main.cpp)
include_directories(application ${Vulkan_INCLUDE_DIRS})
target_link_libraries(application ${Vulkan_LIBRARIES})

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
foreach(SHADER ${SHADERS})
        get_filename_component(SHADER_NAME ${SHADER} NAME)

        add_custom_command(OUTPUT shaders/${SHADER_NAME}.spv
                           COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
                           --target-env vulkan1.2
                           -o shaders/${SHADER_NAME}.spv ${SHADER}
                           DEPENDS ${SHADER}
        )
        target_sources(application PRIVATE shaders/${SHADER_NAME}.spv)
endforeach()
