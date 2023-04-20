cmake_minimum_required (VERSION 3.21)
project(headless_triangle_validation)

find_package(Vulkan REQUIRED)

file(GLOB SHADERS 
  "shader.vert" 
  "shader.frag")

add_executable(headless_triangle_validation main.cpp)
include_directories(headless_triangle_validation ${Vulkan_INCLUDE_DIRS})
target_link_libraries(headless_triangle_validation ${Vulkan_LIBRARIES})

if(DEFINED GLSLANG_VALIDATOR_EXECUTABLE)
  set(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE ${GLSLANG_VALIDATOR_EXECUTABLE})
endif()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders/headless_triangle_validation)
foreach(SHADER ${SHADERS})
        get_filename_component(SHADER_NAME ${SHADER} NAME)

        add_custom_command(
            OUTPUT shaders/headless_triangle_validation/${SHADER_NAME}.spv
            COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
            --target-env vulkan1.2
            -o shaders/headless_triangle_validation/${SHADER_NAME}.spv ${SHADER}
            DEPENDS ${SHADER}
        )
        list(APPEND SHADERS_LIST
            shaders/headless_triangle_validation/${SHADER_NAME}.spv)
endforeach()

if(NOT DEFINED NO_SHADERS)
  add_custom_target(shaders DEPENDS ${SHADERS_LIST})
  add_dependencies(headless_triangle_validation shaders)
  install(DIRECTORY ${CMAKE_BINARY_DIR}/shaders DESTINATION bin)
endif()

install(TARGETS headless_triangle_validation DESTINATION bin)
