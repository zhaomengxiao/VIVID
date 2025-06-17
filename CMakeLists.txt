cmake_minimum_required(VERSION 3.10)
project(VIVID CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(PHYSX_BUILD_TYPE "release" CACHE STRING "The build type of PhysX")

# 统一PhysX路径变量
set(VENDOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor)
set(PHYSX_ROOT "${VENDOR_DIR}/PhysX/physx/install/vc17win64/PhysX")
set(PHYSX_DLL_DIR "${PHYSX_ROOT}/bin/win.x86_64.vc143.md/${PHYSX_BUILD_TYPE}")
set(PHYSX_LIB_DIR "${PHYSX_ROOT}/bin/win.x86_64.vc143.md/${PHYSX_BUILD_TYPE}")
set(PHYSX_INCLUDE_DIR "${PHYSX_ROOT}/include")

# Optick配置
set(OPTICK_ROOT "${VENDOR_DIR}/optick")
set(OPTICK_INCLUDE_DIR "${OPTICK_ROOT}/src")
set(OPTICK_LIB_DIR "${OPTICK_ROOT}/lib")

# 定义源代码目录
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# 添加头文件搜索路径
include_directories(
    ${SOURCE_DIR}
    ${SOURCE_DIR}/components
    ${SOURCE_DIR}/systems
    ${SOURCE_DIR}/core
    ${VENDOR_DIR}/rlImGui
    ${VENDOR_DIR}/imgui-docking
    ${VENDOR_DIR}/imgui-docking/backends
    ${PHYSX_INCLUDE_DIR}
    ${OPTICK_INCLUDE_DIR}
)

# 添加库文件搜索路径
link_directories(
    ${PHYSX_LIB_DIR}
    ${OPTICK_LIB_DIR}
)

# 查找依赖包
find_package(raylib CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)

# PhysX库的查找路径
link_directories(${PHYSX_LIB_DIR})

# 添加主程序
add_executable(${PROJECT_NAME} 
    ${SOURCE_DIR}/main.cpp
    ${VENDOR_DIR}/rlImGui/rlImGui.cpp
    ${VENDOR_DIR}/imgui-docking/imgui_draw.cpp
    ${VENDOR_DIR}/imgui-docking/imgui_tables.cpp
    ${VENDOR_DIR}/imgui-docking/imgui_widgets.cpp
    ${VENDOR_DIR}/imgui-docking/imgui.cpp
    ${VENDOR_DIR}/imgui-docking/imgui_demo.cpp
    ${VENDOR_DIR}/imgui-docking/backends/imgui_impl_opengl3.cpp
)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE 
    raylib
    EnTT::EnTT
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

# 构建后自动拷贝PhysX DLL到可执行文件目录
set(PHYSX_DLLS
    PhysX_64.dll
    PhysXFoundation_64.dll
    PhysXCommon_64.dll
    PhysXCooking_64.dll
    PhysXGpu_64.dll
    PVDRuntime_64.dll
)

# 添加 Optick DLL
set(OPTICK_DLLS
    OptickCore.dll
)

# 复制 PhysX DLLs
foreach(dll ${PHYSX_DLLS})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${PHYSX_DLL_DIR}/${dll}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endforeach()

# 复制 Optick DLLs
foreach(dll ${OPTICK_DLLS})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${OPTICK_ROOT}/build/Release/${dll}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endforeach()
