// SDL3 Hello World Example
// This example demonstrates how to use the new SDL3 callback-based application system

#include <iostream>
#include <utility>

#include "vivid/app/SDL3App.h"
#include "vivid/input/camera_controller.h"
#include "vivid/log/log.h"
#include "vivid/physics/physics_component.h"
#include "vivid/physics/physics_system.h"
#include "vivid/render/render_systems.h"
#include "vivid/ui/ui_system.h"
#include "vivid/window/window_component.h"
#include "vivid/window/window_systems.h"
#include "inspector/inspector_systems.h"

struct MyResource {
  int value;
};

VIVID::RENDER::MeshComponent CreateCubeMesh() {
  std::vector<float> vertices
      = {// positions          // normals
         -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.5f,  -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f,
         0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, -0.5f, 0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,

         -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.5f,  -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  -0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

         -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  -0.5f, 0.5f,  -0.5f, -1.0f, 0.0f,  0.0f,
         -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  -0.5f, -0.5f, 0.5f,  -1.0f, 0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.5f,  0.5f,  -0.5f, 1.0f,  0.0f,  0.0f,
         0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,  0.5f,  -0.5f, 0.5f,  1.0f,  0.0f,  0.0f,

         -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.5f,  -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,
         0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  -0.5f, -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,

         -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,  0.5f,  0.0f,  1.0f,  0.0f};
  std::vector<unsigned int> indices
      = {0,  1,  2,  2,  3,  0,  4,  5,  6,  6,  7,  4,  8,  9,  10, 10, 11, 8,
         12, 13, 14, 14, 15, 12, 16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20};

  return {vertices, indices, indices.size()};
}

// Window setup module - creates custom window before WindowSystems
struct WindowSetup {
  WindowSetup(flecs::world& world) {
    using namespace VIVID::WINDOW;

    // Register module
    world.module<WindowSetup>();

    // Import WindowComponents first to register component types
    world.import <WindowComponents>();

    // Create custom window entity with specific configuration
    WindowContext window_config;
    window_config.title = "VIVID Hello SDL3 with WebGPU Rendering";
    window_config.width = 1024;
    window_config.height = 768;
    window_config.x = SDL_WINDOWPOS_CENTERED;
    window_config.y = SDL_WINDOWPOS_CENTERED;
    window_config.flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    window_config.visible = true;
    window_config.should_close = false;

    world.set<WindowContext>(window_config);

    VividLogger::app_info("Custom window entity 'MainWindow' created (1024x768)");
  }
};

// Scene initialization module - creates cube, light, and camera entities
struct Setup {
  Setup(flecs::world& world) {
    VIVID_LOG_SYSTEM("Registering Setup module...");

    using namespace VIVID::RENDER;
    using namespace VIVID::PHYSICS;

    // Register module
    world.module<Setup>();

    // Import required components
    world.import <RenderComponents>();

    // Register scene initialization system (runs at startup)
    world.system("SceneInitialization").kind(flecs::OnStart).run(sceneInitializationImpl);

    VIVID_LOG_SUCCESS("Setup module registration completed!");
  }

private:
  // Scene initialization system implementation
  static void sceneInitializationImpl(flecs::iter& it) {
    auto world = it.world();
    VividLogger::app_info("=== SceneInitialization system executing ===");
    VividLogger::app_info("Initializing scene entities...");

    // --- Create Cube Entity ---
    auto cubeEntity = world.entity("MyCube");
    cubeEntity.set<VIVID::RENDER::TagComponent>({"MyCube"})
        .set<VIVID::RENDER::TransformComponent>({})
        .set<VIVID::RENDER::MeshComponent>(CreateCubeMesh())
        .set<VIVID::RENDER::MaterialComponent>({
            "D:/ClineWorkSpace/VIVID/build/release/standalone/Release/res/shaders/"
            "BlinnPhong.shader",
            {1.0f, 0.5f, 0.2f}  // Orange color
        });

    VividLogger::app_info("Created cube entity");

    // --- Create Light Entity ---
    auto lightEntity = world.entity("PointLight");
    VIVID::RENDER::TransformComponent lightTransform;
    lightTransform.Position = {1.2f, 1.0f, 2.0f};

    lightEntity.set<VIVID::RENDER::TagComponent>({"PointLight"})
        .set<VIVID::RENDER::TransformComponent>(lightTransform)
        .set<VIVID::RENDER::LightComponent>({});

    VividLogger::app_info("Created light entity at position (1.2, 1.0, 2.0)");

    // --- Create Camera Entity ---
    auto cameraEntity = world.entity("MainCamera");
    VIVID::RENDER::TransformComponent camTransform;
    camTransform.Position = {0.0f, 0.0f, 5.0f};

    cameraEntity.set<VIVID::RENDER::TagComponent>({"MainCamera"})
        .set<VIVID::RENDER::TransformComponent>(camTransform)
        .set<VIVID::RENDER::CameraComponent>({})
        .set<VIVID::RENDER::ViewportComponent>({})
        .set<CameraControllerComponent>({});

    VividLogger::app_info("Created camera entity at position (0.0, 0.0, 5.0)");
    VividLogger::app_info("Scene initialization completed!");
    VividLogger::debug("=== SceneInitialization system finished ===");
  }
};

// Module registration overview display
struct ModuleOverview {
  ModuleOverview(flecs::world& world) {
    // Only show detailed overview in Debug builds to reduce verbosity
#ifndef NDEBUG
    VividLogger::app_info(
        "╔══════════════════════════════════════════════════════════════════════════════╗");
    VividLogger::app_info(
        "║                    🚀 VIVID ENGINE - MODULE REGISTRATION                     ║");
    VividLogger::app_info(
        "║                                                                              ║");
    VividLogger::app_info(
        "║  📋 REGISTRATION ORDER & DEPENDENCIES:                                      ║");
    VividLogger::app_info(
        "║                                                                              ║");
    VividLogger::app_info(
        "║  1️⃣  📦 WindowSystems                                                       ║");
    VividLogger::app_info(
        "║      ├── Provides: WindowContext (singleton)                                 ║");
    VividLogger::app_info(
        "║      ├── Systems: WindowInitialization, WindowUpdate, WindowCleanup          ║");
    VividLogger::app_info(
        "║      └── Phase: OnStart → OnUpdate → Shutdown                               ║");
    VividLogger::app_info(
        "║                                                                              ║");
    VividLogger::app_info(
        "║  2️⃣  📦 Setup                                                               ║");
    VividLogger::app_info(
        "║      ├── Depends: WindowSystems                                              ║");
    VividLogger::app_info(
        "║      ├── Provides: Scene entities (Cube, Light, Camera)                      ║");
    VividLogger::app_info(
        "║      └── System: SceneInitialization (OnStart)                              ║");
    VividLogger::app_info(
        "║                                                                              ║");
    VividLogger::app_info(
        "║  3️⃣  📦 RenderSystems                                                      ║");
    VividLogger::app_info(
        "║      ├── Depends: WindowSystems (WindowContext)                              ║");
    VividLogger::app_info(
        "║      ├── Provides: WebGPUContext (singleton)                                 ║");
    VividLogger::app_info(
        "║      ├── Pipeline: Extract → Prepare → Queue → Sort → Render → UI → Submit  ║");
    VividLogger::app_info(
        "║      ├── Systems: InitWebGPU, SyncScene, RenderMesh, Submit                  ║");
    VividLogger::app_info(
        "║      └── Phase: OnStart → PreUpdate → RenderPhase → SubmitPhase → Shutdown  ║");
    VividLogger::app_info(
        "║                                                                              ║");
    VividLogger::app_info(
        "║  4️⃣  📦 UISystems                                                           ║");
    VividLogger::app_info(
        "║      ├── Depends: WindowSystems (WindowContext)                              ║");
    VividLogger::app_info(
        "║      ├── Depends: RenderSystems (WebGPUContext + RenderUIPhase)              ║");
    VividLogger::app_info(
        "║      ├── Systems: InitImGui, ProcessImGuiEvent, ShowImGuiDemo, RenderImGui   ║");
    VividLogger::app_info(
        "║      └── Phase: OnStart → PreUpdate → RenderUIPhase → Shutdown              ║");
    VividLogger::app_info(
        "╚══════════════════════════════════════════════════════════════════════════════╝");
#else
    VividLogger::app_info("🚀 VIVID Engine - Initializing modules...");
#endif
  }
};

VIVID_SDL3_MAIN(
        .set_app_info("VIVID Hello SDL3 with WebGPU Rendering", "1.0.0", "com.vivid.hello_sdl3")
        // 使用枚举设置其他元数据
        .set_metadata(SDL3MetadataProperty::Creator, "VIVID Engine Team")
        .set_metadata(SDL3MetadataProperty::Copyright, "Copyright (c) 2024 VIVID Engine")
        .set_metadata(SDL3MetadataProperty::Url, "https://github.com/vivid-engine/vivid")
        .set_metadata(SDL3MetadataProperty::Type, SDL3AppType::Application)
        // 自定义属性
        .set_custom_metadata("custom_property", "custom_value")
        // 配置日志系统 - 设置为Debug级别以显示详细日志
        .set_default_log_level(VividLogLevel::Debug)
        .set_log_level(VividLogCategory::Application, VividLogLevel::Debug)
        // 应用配置
        .insert_resource<MyResource>(100)
        .import_module<ModuleOverview>()                // Display module registration overview
        .import_module<VIVID::WINDOW::WindowSystems>()  // Window management (won't create default)
        .import_module<Setup>()                         // Scene initialization (after window)
        // .import_module<WindowSetup>()                   // Create custom window entity first
        .import_module<VIVID::RENDER::RenderSystems>()  // WebGPU rendering (deferred to PreUpdate)
        .import_module<VIVID::UI::UISystems>()          // ImGui UI
        .import_module<editor::inspector::InspectorSystems>()
    // .import_module<VIVID::PHYSICS::PhysicsSystems>()  // Physics simulation

)