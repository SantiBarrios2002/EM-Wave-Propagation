cmake_minimum_required(VERSION 3.16)
project(em_wave LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# --- GLFW ---
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# --- GLM ---
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glm)

# --- GLEW ---
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
FetchContent_Declare(
    glew
    GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
    GIT_TAG        glew-cmake-2.2.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glew)
unset(CMAKE_POLICY_VERSION_MINIMUM CACHE)

find_package(OpenGL REQUIRED)

set(COMMON_LIBS
    glfw
    glm::glm
    libglew_static
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
