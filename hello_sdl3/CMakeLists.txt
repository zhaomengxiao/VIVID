cmake_minimum_required(VERSION 3.14...3.22)

project(HelloSDL3 LANGUAGES CXX)

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

# Set CPM cache options before including CPM
set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/../.cpm_cache" CACHE PATH "CPM source cache directory")
set(CPM_USE_LOCAL_PACKAGES ON CACHE BOOL "Use local packages when available")

include(../cmake/CPM.cmake)

# VIVID engine
CPMAddPackage(NAME VIVID SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../lib)

# ImGui for editor UI - using local vendor copy
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui-docking)    

# OpenGL libraries
# find_package(OpenGL REQUIRED)


# ---- Create standalone executable ----

# Collect all source files including editor
file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

# ImGui source files
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(${PROJECT_NAME} ${sources} ${IMGUI_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 OUTPUT_NAME "HelloSDL3")

target_link_libraries(${PROJECT_NAME} 
    VIVID::VIVID 
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/source
)

# Copy resources to build directory
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#     "${CMAKE_CURRENT_SOURCE_DIR}/res"
#     "$<TARGET_FILE_DIR:${PROJECT_NAME}>/res"
#     COMMENT "Copying resources to build directory"
# )

# Copy required DLLs to executable directory
# if(WIN32)
#     add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#         "$<TARGET_FILE:glfw>"
#         "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
#         COMMENT "Copying GLFW DLL to executable directory"
#     )
    
#     # Copy GLEW DLL if GLEW target exists
#     if(TARGET GLEW::GLEW)
#         add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#             COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             "$<TARGET_FILE:GLEW::GLEW>"
#             "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
#             COMMENT "Copying GLEW DLL to executable directory"
#         )
#     endif()
# endif()

# # Set working directory to executable directory for VS debugging
# set_target_properties(${PROJECT_NAME} PROPERTIES
#     VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
# )
