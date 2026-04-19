cmake_minimum_required(VERSION 3.10)
project(VoxelEditor)

set(CMAKE_CXX_STANDARD 17)

# Important packages
find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(assimp REQUIRED)

# Imgui
include_directories(include src imgui)
set(IMGUI_SOURCES
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_impl_glfw.cpp
    imgui/imgui_impl_opengl3.cpp
)

# Executables
add_executable(main main.cpp src/glad.c ${IMGUI_SOURCES})

# Include directories 
target_include_directories(main PUBLIC 
    include # For glad
    ${ASSIMP_INCLUDE_DIRS}
    path/to/glm
    path/to/stb
)

# Libraries
target_link_libraries(main PRIVATE
    glfw
    OpenGL::GL
    ${ASSIMP_LIBRARIES}
)
