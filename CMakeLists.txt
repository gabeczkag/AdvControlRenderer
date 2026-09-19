cmake_minimum_required(VERSION 3.24)
project(AdvControlRenderer VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT WIN32)
    message(FATAL_ERROR "Adv. Engine currently targets Windows.")
endif()
find_package(Vulkan REQUIRED)
add_executable(AdvControlRenderer WIN32
    src/main.cpp
    src/Core/Application.cpp src/Core/Application.h
    src/Platform/Window.cpp src/Platform/Window.h
    src/Graphics/Renderer.cpp src/Graphics/Renderer.h
    src/Graphics/AdvControl.cpp src/Graphics/AdvControl.h
    src/Graphics/Vulkan/VulkanContext.cpp src/Graphics/Vulkan/VulkanContext.h
    src/Graphics/Vulkan/VulkanRenderer.cpp src/Graphics/Vulkan/VulkanRenderer.h
)
target_compile_definitions(AdvControlRenderer PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN VK_USE_PLATFORM_WIN32_KHR)
target_link_libraries(AdvControlRenderer PRIVATE Vulkan::Vulkan)
find_program(GLSLC glslc HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/Bin32")
if(NOT GLSLC)
    message(FATAL_ERROR "glslc was not found. Install the Vulkan SDK and set VULKAN_SDK.")
endif()
set(SHADER_OUTPUT_DIR "$<TARGET_FILE_DIR:AdvControlRenderer>/shaders")
add_custom_command(TARGET AdvControlRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUTPUT_DIR}"
    COMMAND ${GLSLC} "${CMAKE_SOURCE_DIR}/shaders/triangle.vert" -o "${SHADER_OUTPUT_DIR}/triangle.vert.spv"
    COMMAND ${GLSLC} "${CMAKE_SOURCE_DIR}/shaders/triangle.frag" -o "${SHADER_OUTPUT_DIR}/triangle.frag.spv"
)