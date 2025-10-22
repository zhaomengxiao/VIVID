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

# Copy required system DLLs to executable directory (only for native builds)
# This is necessary because Dawn requires DLLs in the same directory as the executable
if(NOT EMSCRIPTEN)
    vivid_copy_system_dlls_to_target(${PROJECT_NAME})
endif()

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
    
    # Add custom targets for running with emrun
    # This allows VSCode's "Launch Target" to run the WebAssembly with emrun
    add_custom_target(run-emrun
        COMMAND emrun --port 6931 "$<TARGET_FILE:${PROJECT_NAME}>"
        DEPENDS ${PROJECT_NAME}
        COMMENT "Running ${PROJECT_NAME} with emrun on port 6931"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    # Alternative: Run with a simple HTTP server
    add_custom_target(run-http-server
        COMMAND python -m http.server 8000
        DEPENDS ${PROJECT_NAME}
        COMMENT "Starting HTTP server on port 8000 at ${CMAKE_CURRENT_BINARY_DIR}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()
