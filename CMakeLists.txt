cmake_minimum_required(VERSION 3.10)
project(VIVID CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(PHYSX_BUILD_TYPE "release" CACHE STRING "The build type of PhysX")

# --- Dependencies ---
# Using FetchContent to get GLM
include(FetchContent)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

# Finding packages
find_package(EnTT CONFIG REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
# find_package(raylib CONFIG REQUIRED) # Temporarily commented out, as main.cpp now uses GLFW directly.

# --- Paths ---
set(VENDOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor)
set(PHYSX_ROOT "${VENDOR_DIR}/PhysX/physx/install/vc17win64/PhysX")
set(PHYSX_DLL_DIR "${PHYSX_ROOT}/bin/win.x86_64.vc143.md/${PHYSX_BUILD_TYPE}")
set(PHYSX_LIB_DIR "${PHYSX_ROOT}/bin/win.x86_64.vc143.md/${PHYSX_BUILD_TYPE}")
set(PHYSX_INCLUDE_DIR "${PHYSX_ROOT}/include")
set(OPTICK_ROOT "${VENDOR_DIR}/optick")
set(OPTICK_INCLUDE_DIR "${OPTICK_ROOT}/src")
set(OPTICK_LIB_DIR "${OPTICK_ROOT}/lib")
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# --- Include Directories ---
include_directories(
    ${SOURCE_DIR}
    ${PHYSX_INCLUDE_DIR}
    ${OPTICK_INCLUDE_DIR}
    # Kept for future use
    ${VENDOR_DIR}/rlImGui
    ${VENDOR_DIR}/imgui-docking
    ${VENDOR_DIR}/imgui-docking/backends
)

# --- Link Directories ---
link_directories(
    ${PHYSX_LIB_DIR}
    ${OPTICK_LIB_DIR}
)

# --- Source Files ---
# Files for the new renderer
set(OPENGL_WRAPPER_SOURCES
    ${SOURCE_DIR}/opengl/GLErrorHandler.cpp
    ${SOURCE_DIR}/opengl/buffer/IndexBuffer.cpp
    ${SOURCE_DIR}/opengl/buffer/VertexArray.cpp
    ${SOURCE_DIR}/opengl/buffer/VertexBuffer.cpp
    ${SOURCE_DIR}/opengl/shader/Shader.cpp
)

set(IMGUI_SOURCES
    ${VENDOR_DIR}/imgui-docking/imgui.cpp
    ${VENDOR_DIR}/imgui-docking/imgui_draw.cpp
    ${VENDOR_DIR}/imgui-docking/imgui_tables.cpp
    ${VENDOR_DIR}/imgui-docking/imgui_widgets.cpp
    ${VENDOR_DIR}/imgui-docking/backends/imgui_impl_opengl3.cpp
    ${VENDOR_DIR}/imgui-docking/backends/imgui_impl_glfw.cpp
)

add_executable(${PROJECT_NAME} 
    ${SOURCE_DIR}/main.cpp
    ${SOURCE_DIR}/systems/renderer_system.cpp
    ${OPENGL_WRAPPER_SOURCES}
    ${IMGUI_SOURCES}
)

# --- Link Libraries ---
target_link_libraries(${PROJECT_NAME} PRIVATE 
    # New libraries for OpenGL rendering with GLFW
    glfw
    GLEW::glew
    OpenGL::GL
    glm::glm

    # Existing libraries
    EnTT::EnTT
    # raylib # Temporarily commented out, conflicts with glfw
    PhysXFoundation_64
    PhysXCommon_64
    PhysX_64
    PhysXExtensions_static_64
    PhysXPvdSDK_static_64
    PhysXGpu_64
    PhysXCooking_64
    PhysXCharacterKinematic_static_64
    PhysXTask_static_64
    PhysXVehicle2_static_64
    PVDRuntime_64
    OptickCore
)

# --- Set IDE Properties ---
# This ensures that when running from Visual Studio or other IDEs,
# the working directory is set to where the executable is, so it can find the 'res' folder.
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

# --- Post-Build Steps ---
# Copy shader resources
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/res"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/res"
    COMMENT "Copying resources to build directory"
)

# Copy existing DLLs
set(PHYSX_DLLS
    PhysX_64.dll
    PhysXFoundation_64.dll
    PhysXCommon_64.dll
    PhysXCooking_64.dll
    PhysXGpu_64.dll
    PVDRuntime_64.dll
)
set(OPTICK_DLLS
    OptickCore.dll
)
foreach(dll ${PHYSX_DLLS})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${PHYSX_DLL_DIR}/${dll}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endforeach()
foreach(dll ${OPTICK_DLLS})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${OPTICK_ROOT}/build/Release/${dll}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endforeach()
