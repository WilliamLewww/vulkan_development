cmake_minimum_required (VERSION 3.21)
project(headless_triangle)

find_package(Vulkan REQUIRED)

if(DEFINED VALIDATION_ENABLED)
  add_compile_definitions(VALIDATION_ENABLED=1)
endif()

if(DEFINED GLSLANG_VALIDATOR_EXECUTABLE)
  set(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE ${GLSLANG_VALIDATOR_EXECUTABLE})
endif()

add_compile_definitions(SHARE_PATH="${CMAKE_INSTALL_PREFIX}/share")

add_executable(headless_triangle main.cpp)
include_directories(headless_triangle ${Vulkan_INCLUDE_DIRS})
target_link_libraries(headless_triangle ${Vulkan_LIBRARIES})
set_property(TARGET headless_triangle PROPERTY CXX_STANDARD 20)

file(GLOB SHADERS 
  "shader.vert" 
  "shader.frag")

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders/headless_triangle)
foreach(SHADER ${SHADERS})
        get_filename_component(SHADER_NAME ${SHADER} NAME)

        add_custom_command(
            OUTPUT shaders/headless_triangle/${SHADER_NAME}.spv
            COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
            --target-env vulkan1.2
            -o shaders/headless_triangle/${SHADER_NAME}.spv ${SHADER}
            DEPENDS ${SHADER}
        )
        list(APPEND SHADERS_LIST
            shaders/headless_triangle/${SHADER_NAME}.spv)
endforeach()

add_custom_target(shaders DEPENDS ${SHADERS_LIST})
add_dependencies(headless_triangle shaders)
install(DIRECTORY ${CMAKE_BINARY_DIR}/shaders DESTINATION share)

install(TARGETS headless_triangle DESTINATION bin)
