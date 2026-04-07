cmake_minimum_required(VERSION 3.10)
project(VoxelEditor)

set(CMAKE_CXX_STANDARD 17)

# --- Find Packages ---
find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(assimp REQUIRED)

# --- Add Your Executable ---
add_executable(main main.cpp src/glad.c)

# --- Include Directories ---
target_include_directories(main PUBLIC 
    include # For glad
    ${ASSIMP_INCLUDE_DIRS}
    path/to/glm
    path/to/stb
)

# --- Link Libraries ---
target_link_libraries(main PRIVATE
    glfw
    OpenGL::GL
    ${ASSIMP_LIBRARIES}
)
