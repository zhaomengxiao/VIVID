// SDL3 Hello World Example
// This example demonstrates how to use the new SDL3 callback-based application system

#include "inspector/inspector_systems.h"
#include "vivid/app/SDL3App.h"
#include "vivid/input/camera_controller.h"
#include "vivid/input/input_system.h"
#include "vivid/log/log.h"
#include "vivid/render/render_systems.h"
#include "vivid/ui/ui_system.h"
#include "vivid/window/window_component.h"
#include "vivid/window/window_systems.h"

struct MyResource {
  int value_;
};

namespace {
vivid::render::MeshComponent CreateCubeMesh() {
  std::vector<float> const kVertices
      = {// positions          // normals
         -0.5F, -0.5F, -0.5F, 0.0F,  0.0F,  -1.0F, 0.5F,  -0.5F, -0.5F, 0.0F,  0.0F,  -1.0F,
         0.5F,  0.5F,  -0.5F, 0.0F,  0.0F,  -1.0F, -0.5F, 0.5F,  -0.5F, 0.0F,  0.0F,  -1.0F,

         -0.5F, -0.5F, 0.5F,  0.0F,  0.0F,  1.0F,  0.5F,  -0.5F, 0.5F,  0.0F,  0.0F,  1.0F,
         0.5F,  0.5F,  0.5F,  0.0F,  0.0F,  1.0F,  -0.5F, 0.5F,  0.5F,  0.0F,  0.0F,  1.0F,

         -0.5F, 0.5F,  0.5F,  -1.0F, 0.0F,  0.0F,  -0.5F, 0.5F,  -0.5F, -1.0F, 0.0F,  0.0F,
         -0.5F, -0.5F, -0.5F, -1.0F, 0.0F,  0.0F,  -0.5F, -0.5F, 0.5F,  -1.0F, 0.0F,  0.0F,

         0.5F,  0.5F,  0.5F,  1.0F,  0.0F,  0.0F,  0.5F,  0.5F,  -0.5F, 1.0F,  0.0F,  0.0F,
         0.5F,  -0.5F, -0.5F, 1.0F,  0.0F,  0.0F,  0.5F,  -0.5F, 0.5F,  1.0F,  0.0F,  0.0F,

         -0.5F, -0.5F, -0.5F, 0.0F,  -1.0F, 0.0F,  0.5F,  -0.5F, -0.5F, 0.0F,  -1.0F, 0.0F,
         0.5F,  -0.5F, 0.5F,  0.0F,  -1.0F, 0.0F,  -0.5F, -0.5F, 0.5F,  0.0F,  -1.0F, 0.0F,

         -0.5F, 0.5F,  -0.5F, 0.0F,  1.0F,  0.0F,  0.5F,  0.5F,  -0.5F, 0.0F,  1.0F,  0.0F,
         0.5F,  0.5F,  0.5F,  0.0F,  1.0F,  0.0F,  -0.5F, 0.5F,  0.5F,  0.0F,  1.0F,  0.0F};
  std::vector<unsigned int> const kIndices
      = {0,  1,  2,  2,  3,  0,  4,  5,  6,  6,  7,  4,  8,  9,  10, 10, 11, 8,
         12, 13, 14, 14, 15, 12, 16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20};

  return {kVertices, kIndices, kIndices.size()};
}
}  // namespace

// Window setup module - creates custom window before WindowSystems
struct WindowSetup {
  explicit WindowSetup(flecs::world& world) {
    using vivid::window::WindowComponents;
    using vivid::window::WindowContext;

    // Register module
    world.module<WindowSetup>();

    // Import WindowComponents first to register component types
    world.import <vivid::window::WindowComponents>();

    // Create custom window entity with specific configuration
    vivid::window::WindowContext window_config;
    window_config.title_ = "VIVID Hello SDL3 with WebGPU Rendering";
    window_config.width_ = 1024;
    window_config.height_ = 768;
    window_config.x_ = SDL_WINDOWPOS_CENTERED;
    window_config.y_ = SDL_WINDOWPOS_CENTERED;
    window_config.flags_ = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    window_config.visible_ = true;
    window_config.should_close_ = false;

    world.set<vivid::window::WindowContext>(window_config);

    VividLogger::app_info("Custom window entity 'MainWindow' created (1024x768)");
  }
};

// Scene initialization module - creates cube, light, and camera entities
struct Setup {
  explicit Setup(flecs::world& world) {
    VIVID_LOG_SYSTEM("Registering Setup module...");

    using vivid::render::RenderComponents;

    // Register module
    world.module<Setup>();

    // Import required components
    world.import <vivid::render::RenderComponents>();

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
    auto cube_entity = world.entity("MyCube");
    cube_entity.set<vivid::render::TagComponent>({"MyCube"})
        .set<vivid::render::TransformComponent>({})
        .set<vivid::render::MeshComponent>(CreateCubeMesh())
        .set<vivid::render::MaterialComponent>({
            "D:/ClineWorkSpace/VIVID/build/release/standalone/Release/res/shaders/"
            "BlinnPhong.shader",
            {1.0F, 0.5F, 0.2F}  // Orange color
        });

    VividLogger::app_info("Created cube entity");

    // --- Create Light Entity ---
    auto light_entity = world.entity("PointLight");
    vivid::render::TransformComponent light_transform;
    light_transform.position_ = {1.2F, 1.0F, 2.0F};

    light_entity.set<vivid::render::TagComponent>({"PointLight"})
        .set<vivid::render::TransformComponent>(light_transform)
        .set<vivid::render::LightComponent>({});

    VividLogger::app_info("Created light entity at position (1.2, 1.0, 2.0)");

    // --- Create Camera Entity ---
    auto camera_entity = world.entity("MainCamera");
    vivid::render::TransformComponent cam_transform;
    cam_transform.position_ = {0.0F, 0.0F, 5.0F};

    camera_entity.set<vivid::render::TagComponent>({"MainCamera"})
        .set<vivid::render::TransformComponent>(cam_transform)
        .set<vivid::render::CameraComponent>({})
        .set<vivid::render::ViewportComponent>({})
        .set<CameraControllerComponent>({});

    VividLogger::app_info("Created camera entity at position (0.0, 0.0, 5.0)");
    VividLogger::app_info("Scene initialization completed!");
    VividLogger::debug("=== SceneInitialization system finished ===");
  }
};

// Module registration overview display
struct ModuleOverview {
  explicit ModuleOverview([[maybe_unused]] flecs::world& world) {
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

namespace vivid::app {
SDL3AppBuilder CreateAppInstance() {
  return CreateAppInstance([](SDL3AppBuilder& builder) {
    builder
        .set_app_info("VIVID Hello SDL3 with WebGPU Rendering", "1.0.0", "com.vivid.hello_sdl3")
        // 使用枚举设置其他元数据
        .set_metadata(SDL3MetadataProperty::kCreator, "VIVID Engine Team")
        .set_metadata(SDL3MetadataProperty::kCopyright, "Copyright (c) 2024 VIVID Engine")
        .set_metadata(SDL3MetadataProperty::kUrl, "https://github.com/vivid-engine/vivid")
        .set_metadata(SDL3MetadataProperty::kType, sdl3_app_type::kApplication)
        // 自定义属性
        .set_custom_metadata("custom_property", "custom_value")
        // 配置日志系统 - 设置为Debug级别以显示详细日志
        .set_default_log_level(VividLogLevel::Debug)
        .set_log_level(VividLogCategory::Application, VividLogLevel::Debug)
        // 应用配置
        .insert_resource<MyResource>(100)
        .import_module<ModuleOverview>()                // Display module registration overview
        .import_module<vivid::window::WindowSystems>()  // Window management (won't create default)
        .import_module<Setup>()                         // Scene initialization (after window)
        // .import_module<WindowSetup>()                   // Create custom window entity first
        .import_module<vivid::render::RenderSystems>()  // WebGPU rendering (deferred to PreUpdate)
        .import_module<vivid::ui::UISystems>()          // ImGui UI
        .import_module<vivid::input::InputSystems>()    // Input processing and camera control
        .import_module<editor::inspector::InspectorSystems>();
    // .import_module<VIVID::PHYSICS::PhysicsSystems>()  // Physics simulation
  });
}
}  // namespace vivid::app