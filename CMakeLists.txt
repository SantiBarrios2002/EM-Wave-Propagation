cmake_minimum_required(VERSION 3.16)
project(em_wave LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GLEW REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

set(COMMON_LIBS
    GLEW::GLEW
    glfw
    glm::glm
    OpenGL::GL
)

# Phase 1: 2D wave simulation
add_executable(2D_wave 2D_wave.cpp)
target_include_directories(2D_wave PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(2D_wave PRIVATE ${COMMON_LIBS})

# Copy shaders to build directory
add_custom_command(TARGET 2D_wave POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:2D_wave>/shaders
)
