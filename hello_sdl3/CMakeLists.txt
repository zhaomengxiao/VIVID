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


# ---- Create standalone executable ----

# Collect all source files including editor
file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

add_executable(${PROJECT_NAME} ${sources})

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 OUTPUT_NAME "HelloSDL3")

target_link_libraries(${PROJECT_NAME} 
    VIVID::VIVID 
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source
)

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "index")
    add_compile_options(-sDISABLE_EXCEPTION_CATCHING=1 -DIMGUI_DISABLE_FILE_FUNCTIONS=1)
   
    target_link_options(${PROJECT_NAME} PRIVATE
        "--use-port=emdawnwebgpu"
        "-sUSE_SDL=3"  # There is currently no official support internally at emscripten
        "-sWASM=1"
        "-sASYNCIFY=1"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sSTACK_SIZE=1mb"
        "-sEXPORTED_RUNTIME_METHODS=cwrap"
        # "-sMODULARIZE=1"
        "-sNO_EXIT_RUNTIME=0"
        "-sASSERTIONS=1"
        "-sDISABLE_EXCEPTION_CATCHING=1"
        #"-sNO_FILESYSTEM=1"  # sdl3 integrates a filesystem, you need to disable it at SDL3 build time
        "--shell-file=${CMAKE_CURRENT_LIST_DIR}/../emscripten/shell_minimal.html"
    )
endif()

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
