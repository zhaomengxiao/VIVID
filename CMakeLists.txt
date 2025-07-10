cmake_minimum_required(VERSION 3.10)
project(VIVID CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Dependencies ---
# Using FetchContent to get GLM and EnTT
include(FetchContent)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
FetchContent_Declare(
    EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG master
)
FetchContent_MakeAvailable(glm EnTT)

# Finding packages
find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

# --- Paths ---
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# --- Include Directories ---
include_directories(
    ${SOURCE_DIR}
)

# --- Source Files ---
set(OPENGL_WRAPPER_SOURCES
    ${SOURCE_DIR}/opengl/GLErrorHandler.cpp
    ${SOURCE_DIR}/opengl/buffer/IndexBuffer.cpp
    ${SOURCE_DIR}/opengl/buffer/VertexArray.cpp
    ${SOURCE_DIR}/opengl/buffer/VertexBuffer.cpp
    ${SOURCE_DIR}/opengl/shader/Shader.cpp
)

# Test compilation with basic structure
add_executable(${PROJECT_NAME} 
    ${SOURCE_DIR}/main.cpp
    ${SOURCE_DIR}/rendering/render_system.cpp
    ${OPENGL_WRAPPER_SOURCES}
    src/editor/SceneHierarchyPanel.cpp
    src/editor/InspectorPanel.cpp
    src/editor/ComponentRegistry.cpp
    src/plugins/DefaultPlugin.cpp
    src/rendering/render_plugin.cpp
    src/editor/editor_plugin.cpp
)

# --- Link Libraries ---
target_link_libraries(${PROJECT_NAME} PRIVATE 
    glfw
    GLEW::glew
    OpenGL::GL
    glm::glm
    EnTT::EnTT
)

# --- Set IDE Properties ---
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

# --- Post-Build Steps ---
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/res"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/res"
    COMMENT "Copying resources to build directory"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SOURCE_DIR}
)
