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

struct MyResource {
  int value;
};

VIVID::RENDER::MeshComponent CreateCubeMesh() {
  // Cube vertices with correct winding order (CCW when viewed from outside)
  // Each vertex: position (3 floats) + normal (3 floats) = 6 floats
  std::vector<float> vertices = {
      // Back face (z = -0.5) - viewed from +z direction, CCW order
      -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,  // 0: bottom-left
      0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,   // 1: bottom-right
      0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,    // 2: top-right
      -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,   // 3: top-left

      // Front face (z = 0.5) - viewed from +z direction (camera side), CCW order
      -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,  // 4: bottom-left
      0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,   // 5: bottom-right
      0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,    // 6: top-right
      -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,   // 7: top-left

      // Left face (x = -0.5) - viewed from +x direction, CCW order
      -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,  // 8: bottom-back
      -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f,   // 9: top-back
      -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,    // 10: top-front
      -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f,   // 11: bottom-front

      // Right face (x = 0.5) - viewed from -x direction, CCW order
      0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,  // 12: bottom-back
      0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,   // 13: bottom-front
      0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,    // 14: top-front
      0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,   // 15: top-back

      // Bottom face (y = -0.5) - viewed from +y direction, CCW order
      -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,  // 16: back-left
      -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,   // 17: front-left
      0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,    // 18: front-right
      0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,   // 19: back-right

      // Top face (y = 0.5) - viewed from -y direction, CCW order
      -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,  // 20: back-left
      0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,   // 21: back-right
      0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,    // 22: front-right
      -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f    // 23: front-left
  };

  // Indices for each face (2 triangles per face, CCW winding)
  std::vector<unsigned int> indices = {// Back face
                                       0, 1, 2, 2, 3, 0,
                                       // Front face
                                       4, 5, 6, 6, 7, 4,  // CCW from camera (+z direction)
                                                          // Left face
                                       8, 9, 10, 10, 11, 8,
                                       // Right face
                                       12, 13, 14, 14, 15, 12,
                                       // Bottom face
                                       16, 17, 18, 18, 19, 16,
                                       // Top face
                                       20, 21, 22, 22, 23, 20};

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

    // --- Create Camera Entity for Render Window ---
    // Entities with both CameraComponent and ViewportComponent will automatically
    // render to an ImGui window. The window title will be from TagComponent.Tag.
    auto cameraEntity = world.entity("MainCamera");
    VIVID::RENDER::TransformComponent camTransform;
    // Move camera closer to cube for better perspective effect
    // Position at (0, 0, 3) instead of (0, 0, 5) to make perspective more visible
    camTransform.Position = {0.0f, 0.0f, 3.0f};

    // Setup ViewportComponent with initial size for ImGui window
    // The size will automatically adjust based on ImGui window size
    VIVID::RENDER::ViewportComponent viewport;
    viewport.Width = 800.0f;
    viewport.Height = 600.0f;

    cameraEntity.set<VIVID::RENDER::TagComponent>({"MainCamera"})
        .set<VIVID::RENDER::TransformComponent>(camTransform)
        .set<VIVID::RENDER::CameraComponent>({})
        .set<VIVID::RENDER::ViewportComponent>(viewport)  // Enables render window in ImGui
        .set<CameraControllerComponent>({});

    VividLogger::app_info("Created camera entity at position (0.0, 0.0, 3.0)");
    VividLogger::app_info("Render window will appear in ImGui with title 'MainCamera'");

    // --- Create Second Camera Entity with 45-degree angle view ---
    // This camera will render from a diagonal angle (3, 3, 3) looking at the origin
    auto sideCameraEntity = world.entity("SideCamera");
    VIVID::RENDER::TransformComponent sideCamTransform;
    sideCamTransform.Position = {3.0f, 3.0f, 3.0f};  // Position at diagonal angle

    // Setup ViewportComponent for the side camera
    VIVID::RENDER::ViewportComponent sideViewport;
    sideViewport.Width = 800.0f;
    sideViewport.Height = 600.0f;

    // Setup CameraControllerComponent to look at origin (0, 0, 0)
    // Front vector points from (3, 3, 3) to (0, 0, 0) = (-1, -1, -1), normalized
    CameraControllerComponent sideCameraController;
    sideCameraController.Front = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
    sideCameraController.WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    // Calculate Right and Up vectors based on Front and WorldUp
    sideCameraController.Right
        = glm::normalize(glm::cross(sideCameraController.Front, sideCameraController.WorldUp));
    sideCameraController.Up
        = glm::normalize(glm::cross(sideCameraController.Right, sideCameraController.Front));

    sideCameraEntity.set<VIVID::RENDER::TagComponent>({"SideCamera"})
        .set<VIVID::RENDER::TransformComponent>(sideCamTransform)
        .set<VIVID::RENDER::CameraComponent>({})
        .set<VIVID::RENDER::ViewportComponent>(sideViewport)  // Enables render window in ImGui
        .set<CameraControllerComponent>(sideCameraController);

    VividLogger::app_info("Created side camera entity at position (3.0, 3.0, 3.0)");
    VividLogger::app_info("Render window will appear in ImGui with title 'SideCamera'");
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
    // .import_module<VIVID::PHYSICS::PhysicsSystems>()  // Physics simulation

)