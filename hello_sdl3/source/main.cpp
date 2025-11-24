// SDL3 Hello World Example
// This example demonstrates how to use the new SDL3 callback-based application system

#include <algorithm>
#include <array>
#include <string>

#include "imgui.h"
#include "vivid/app/SDL3App.h"
#include "vivid/input/camera_controller.h"
#include "vivid/input/input_system.h"
#include "vivid/input/input_visual_debug_systems.h"
#include "vivid/log/log.h"
#include "vivid/render/render_systems.h"
#include "vivid/ui/ui_system.h"
#include "vivid/window/window_component.h"
#include "vivid/window/window_systems.h"

struct MyResource {
  int value_;
};

#include "vivid/mesh/mesh_generator.h"

// Window setup module - creates custom window before WindowSystems
struct WindowSetup {
  explicit WindowSetup(flecs::world& world) {
    using vivid::window::WindowContext;

    // Register module
    world.module<WindowSetup>();

    // Import WindowComponents first to register component types
    world.import <vivid::window::WindowComponents>();

    // Create custom window entity with specific configuration
    WindowContext window_config;
    window_config.title_ = "VIVID Hello SDL3 with WebGPU Rendering";
    window_config.width_ = 1024;
    window_config.height_ = 768;
    window_config.x_ = SDL_WINDOWPOS_CENTERED;
    window_config.y_ = SDL_WINDOWPOS_CENTERED;
    window_config.flags_ = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    window_config.visible_ = true;
    window_config.should_close_ = false;

    world.set<WindowContext>(window_config);

    VividLogger::app_info("Custom window entity 'MainWindow' created (1024x768)");
  }
};

namespace {
void print_value(const flecs::world& world, const flecs::cursor& cur) {
  // Get unit entity and component
  const flecs::entity kUnit = cur.get_unit();
  const auto& u_data = kUnit.get<flecs::Unit>();

  // 获取成员实体以访问元数据（如 Range）
  const flecs::entity kMember = world.entity(ecs_meta_get_member_id(&cur.cursor_));
  if (kMember.is_valid()) {
    if (kMember.has<flecs::MemberRanges>()) {
      const auto& range = kMember.get<flecs::MemberRanges>();
      ImGui::Text("Range: %f - %f", range.value.min, range.value.max);
    } else {
      ImGui::Text("No Range");
    }

  } else {
    ImGui::Text("Invalid Member");
  }

  const flecs::entity kRgbUnit = world.entity<flecs::units::color::Rgb>();

  // Print value with unit symbol
  //   std::cout << cur.get_member() << ": " << cur.get_float() << " "
  //             << (u_data.symbol ? u_data.symbol : "") << "\n";
  const char* symbol = nullptr;
  if (kUnit == kRgbUnit) {
    symbol = const_cast<char*>("RGB");
  } else {
    symbol = u_data.symbol;
  }

  ImGui::Separator();

  ImGui::Text("%s", cur.get_member().c_str());
  ImGui::Text("%f", cur.get_float());
  ImGui::Text("%s", symbol);
}
}  // namespace
// Scene initialization module - creates cube, light, and camera entities
struct RotateComponent {
  float speed_ = 1.0F;
};

// Scene initialization module - creates cube, light, and camera entities
struct Setup {
  explicit Setup(flecs::world& world) {
    VividLogger::app_info("Registering Setup module...");

    // Register module
    world.module<Setup>();

    // Import required components
    world.import <vivid::render::RenderComponents>();
    world.import <vivid::input::InputComponents>();

    // Register component
    world.component<RotateComponent>();

    // Register scene initialization system (runs at startup)
    world.system("SceneInitialization").kind(flecs::OnStart).run(sceneInitializationImpl);

    // Register rotation system
    world.system<vivid::render::TransformComponent, const RotateComponent>("RotateSystem")
        .each([](vivid::render::TransformComponent& t, const RotateComponent& r) {
          t.rotation_.z += 0.016F * r.speed_;  // Simple rotation, assuming ~60fps or use delta_time
                                               // if available in iter
        });

    VividLogger::app_info("Setup module registration completed!");

    // Moved from SceneInitialization system to guarantee immediate component addition
    const auto kTestEntity = world.entity("TestEntity");
    kTestEntity.ensure<vivid::render::Color3f>();
    VividLogger::app_info("TestEntity: %s", world.to_json(&kTestEntity).c_str());
  }

private:
  // Scene initialization system implementation
  static void sceneInitializationImpl(flecs::iter& it) {
    auto world = it.world();
    VividLogger::app_info("=== SceneInitialization system executing ===");
    VividLogger::app_info("Initializing scene entities...");

    // --- Create 9 Cubes in 3x3 Grid ---
    const float kSpacing = 1.5F;
    for (int x = 0; x < 3; ++x) {
      for (int y = 0; y < 3; ++y) {
        std::string const kName = "Cube_" + std::to_string(x) + "_" + std::to_string(y);
        const auto kCubeEntity = world.entity(kName.c_str());

        vivid::render::TransformComponent transform;
        transform.position_ = {kSpacing * static_cast<float>(x - 1),  // -1.5, 0, 1.5
                               kSpacing * static_cast<float>(y - 1),  // -1.5, 0, 1.5
                               0.0F};

        kCubeEntity.set<vivid::render::TagComponent>({kName})
            .set<vivid::render::TransformComponent>(transform)
            .set<vivid::render::MeshComponent>(
                vivid::mesh::MeshGenerator::CreateCube(1.0F, 1.0F, 1.0F))
            .set<vivid::render::MaterialComponent>({
                "D:/ClineWorkSpace/VIVID/build/release/standalone/Release/res/shaders/"
                "BlinnPhong.shader",
                {1.0F, 0.5F + (static_cast<float>(x) * 0.2F),
                 0.2F + (static_cast<float>(y) * 0.2F)}  // Varying colors
            })
            .set<RotateComponent>({1.0F + static_cast<float>(x + y)});  // Varying speeds
      }
    }

    VividLogger::app_info("Created 9 cube entities");

    // --- Create Plane Entity ---
    auto plane_entity = world.entity("TestPlane");
    vivid::render::TransformComponent plane_transform;
    plane_transform.position_ = {0.0F, -2.0F, 0.0F};

    plane_entity.set<vivid::render::TagComponent>({"TestPlane"})
        .set<vivid::render::TransformComponent>(plane_transform)
        .set<vivid::render::MeshComponent>(vivid::mesh::MeshGenerator::CreatePlane(10.0F, 10.0F))
        .set<vivid::render::MaterialComponent>({
            "D:/ClineWorkSpace/VIVID/build/release/standalone/Release/res/shaders/"
            "BlinnPhong.shader",
            {0.5F, 0.5F, 0.5F}  // Gray color
        });
    VividLogger::app_info("Created plane entity");

    // --- Create Sphere Entity ---
    auto sphere_entity = world.entity("TestSphere");
    vivid::render::TransformComponent sphere_transform;
    sphere_transform.position_ = {3.0F, 0.0F, 0.0F};

    sphere_entity.set<vivid::render::TagComponent>({"TestSphere"})
        .set<vivid::render::TransformComponent>(sphere_transform)
        .set<vivid::render::MeshComponent>(vivid::mesh::MeshGenerator::CreateSphere(0.8F, 32, 32))
        .set<vivid::render::MaterialComponent>({
            "D:/ClineWorkSpace/VIVID/build/release/standalone/Release/res/shaders/"
            "BlinnPhong.shader",
            {0.2F, 0.8F, 0.2F}  // Green color
        });
    VividLogger::app_info("Created sphere entity");

    // --- Create Light Entity ---
    const auto kLightEntity = world.entity("PointLight");
    vivid::render::TransformComponent light_transform;
    light_transform.position_ = {1.2F, 1.0F, 5.0F};  // Moved light back a bit

    kLightEntity.set<vivid::render::TagComponent>({"PointLight"})
        .set<vivid::render::TransformComponent>(light_transform)
        .set<vivid::render::LightComponent>({});

    VividLogger::app_info("Created light entity at position (1.2, 1.0, 5.0)");

    // --- Create Camera Entity for Render Window ---
    // Entities with both CameraComponent and ViewportComponent will automatically
    // render to an ImGui window. The window title will be from TagComponent.Tag.
    const auto kCameraEntity = world.entity("MainCamera");
    vivid::render::TransformComponent cam_transform;
    // Move camera back to see all cubes
    cam_transform.position_ = {0.0F, 0.0F, 8.0F};

    // Setup ViewportComponent with initial size for ImGui window
    // The size will automatically adjust based on ImGui window size
    vivid::render::ViewportComponent viewport;
    viewport.width_ = 800.0F;
    viewport.height_ = 600.0F;

    kCameraEntity.set<vivid::render::TagComponent>({"MainCamera"})
        .set<vivid::render::TransformComponent>(cam_transform)
        .set<vivid::render::CameraComponent>({})
        .set<vivid::render::ViewportComponent>(viewport)  // Enables render window in ImGui
        .set<CameraControllerComponent>({});

    VividLogger::app_info("Created camera entity at position (0.0, 0.0, 8.0)");
    VividLogger::app_info("Render window will appear in ImGui with title 'MainCamera'");

    // --- Create Second Camera Entity with 45-degree angle view ---
    // This camera will render from a diagonal angle (3, 3, 3) looking at the origin
    const auto kSideCameraEntity = world.entity("SideCamera");
    vivid::render::TransformComponent side_cam_transform;
    side_cam_transform.position_ = {5.0F, 5.0F, 5.0F};  // Position at diagonal angle, further out

    // Setup ViewportComponent for the side camera
    vivid::render::ViewportComponent side_viewport;
    side_viewport.width_ = 800.0F;
    side_viewport.height_ = 600.0F;

    // Setup CameraControllerComponent to look at origin (0, 0, 0)
    // Front vector points from (5, 5, 5) to (0, 0, 0) = (-1, -1, -1), normalized
    CameraControllerComponent side_camera_controller;
    side_camera_controller.front_ = glm::normalize(glm::vec3(-1.0F, -1.0F, -1.0F));
    side_camera_controller.world_up_ = glm::vec3(0.0F, 1.0F, 0.0F);
    // Calculate Right and Up vectors based on Front and WorldUp
    side_camera_controller.right_ = glm::normalize(
        glm::cross(side_camera_controller.front_, side_camera_controller.world_up_));
    side_camera_controller.up_
        = glm::normalize(glm::cross(side_camera_controller.right_, side_camera_controller.front_));

    // Calculate initial Yaw and Pitch from Front vector to synchronize with mouse controls
    // Pitch = asin(front.y), Yaw = atan2(front.z, front.x)
    side_camera_controller.pitch_ = glm::degrees(asin(side_camera_controller.front_.y));
    side_camera_controller.yaw_
        = glm::degrees(atan2(side_camera_controller.front_.z, side_camera_controller.front_.x));

    kSideCameraEntity.set<vivid::render::TagComponent>({"SideCamera"})
        .set<vivid::render::TransformComponent>(side_cam_transform)
        .set<vivid::render::CameraComponent>({})
        .set<vivid::render::ViewportComponent>(side_viewport)  // Enables render window in ImGui
        .set<CameraControllerComponent>(side_camera_controller);

    VividLogger::app_info("Created side camera entity at position (5.0, 5.0, 5.0)");
    VividLogger::app_info("Render window will appear in ImGui with title 'SideCamera'");
    VividLogger::app_info("Scene initialization completed!");
    VividLogger::debug("=== SceneInitialization system finished ===");
  }
};

struct ImGuiDemo {
  explicit ImGuiDemo(flecs::world& world) {
    world.module<ImGuiDemo>();

    world.system("ImGuiDemo").kind(flecs::OnUpdate).run(ImGuiDemoImpl);
  }

private:
  static void ImGuiDemoImpl(flecs::iter& it) {
    // Static state for demo windows
    static bool show_demo_window = true;
    static bool show_another_window = false;
    static ImVec4 clear_color = ImVec4(0.45F, 0.55F, 0.60F, 1.00F);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named
    // window.

    // Our state

    static float f = 0.0F;
    static int counter = 0;

    ImGui::Begin("Hello, world!");  // Create a window called "Hello, world!" and append into it.

    // get testEntity
    const flecs::entity kTestEntity = it.world().lookup("Setup::TestEntity");

    // Use cursor API to print values with units
    // Create cursor for the component
    auto& color_data = kTestEntity.ensure<vivid::render::Color3f>();
    flecs::cursor cur = it.world().cursor<vivid::render::Color3f>(&color_data);
    cur.push();
    print_value(it.world(), cur);
    cur.next();
    print_value(it.world(), cur);
    cur.next();
    print_value(it.world(), cur);
    cur.pop();
    std::string const kJson = std::string(it.world().to_json(&color_data).c_str());
    ImGui::Text("%s", kJson.c_str());

    ImGui::Separator();
    // Serialize world to JSON
    static std::array<char, 65536> world_json_buffer{};  // 64KB buffer for JSON text
    std::string world_json = std::string(it.world().to_json().c_str());

    // Format JSON with basic indentation for better readability
    std::string formatted_json;
    int indent_level = 0;
    const std::string kIndentStr = "  ";  // 2 spaces per indent level
    bool in_string = false;
    bool escape_next = false;

    for (size_t i = 0; i < world_json.size(); ++i) {
      const char kChar = world_json[i];

      if (escape_next) {
        formatted_json += kChar;
        escape_next = false;
        continue;
      }

      if (kChar == '\\') {
        escape_next = true;
        formatted_json += kChar;
        continue;
      }

      if (kChar == '"') {
        in_string = !in_string;
        formatted_json += kChar;
        continue;
      }

      if (in_string) {
        formatted_json += kChar;
        continue;
      }

      // Format based on JSON structure
      if (kChar == '{' || kChar == '[') {
        formatted_json += kChar;
        formatted_json += '\n';
        indent_level++;
        for (int j = 0; j < indent_level; ++j) {
          formatted_json += kIndentStr;
        }
      } else if (kChar == '}' || kChar == ']') {
        formatted_json += '\n';
        indent_level--;
        for (int j = 0; j < indent_level; ++j) {
          formatted_json += kIndentStr;
        }
        formatted_json += kChar;
      } else if (kChar == ',') {
        formatted_json += kChar;
        formatted_json += '\n';
        for (int j = 0; j < indent_level; ++j) {
          formatted_json += kIndentStr;
        }
      } else if (kChar == ':') {
        formatted_json += kChar;
        formatted_json += ' ';
      } else if (kChar == ' ' || kChar == '\n' || kChar == '\t') {
        // Skip whitespace outside strings
        continue;
      } else {
        formatted_json += kChar;
      }
    }

    // Copy formatted JSON to buffer
    const size_t kJsonSize = formatted_json.size();
    const size_t kCopySize = std::min(kJsonSize, world_json_buffer.size() - 1);
    formatted_json.copy(world_json_buffer.data(), kCopySize);
    world_json_buffer.at(kCopySize) = '\0';
    if (kJsonSize >= world_json_buffer.size() - 1) {
      constexpr size_t kEllipsisOffset = 3;
      world_json_buffer[world_json_buffer.size() - kEllipsisOffset - 1] = '.';
      world_json_buffer[world_json_buffer.size() - kEllipsisOffset] = '.';
      world_json_buffer[world_json_buffer.size() - kEllipsisOffset + 1] = '.';
    }

    const ImVec2 kTextSize = ImGui::GetContentRegionAvail();
    ImGui::InputTextMultiline("##WorldJson", world_json_buffer.data(), world_json_buffer.size(),
                              kTextSize, ImGuiInputTextFlags_ReadOnly);

    ImGui::Text(
        "This is some useful text.");  // Display some text (you can use a format strings too)
    ImGui::Checkbox("Demo Window",
                    &show_demo_window);  // Edit bools storing our window open/close state
    ImGui::Checkbox("Another Window", &show_another_window);

    ImGui::SliderFloat("float", &f, 0.0F, 1.0F);  // Edit 1 float using a slider
    ImGui::ColorEdit3("clear color", reinterpret_cast<float*>(
                                         &clear_color));  // Edit 3 floats representing a color

    if (ImGui::Button("Button")) {  // Buttons return true when clicked (most widgets return true
                                    // when edited/activated)
      counter++;
    }
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0F / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::End();

    if (show_demo_window) {
      ImGui::ShowDemoWindow();
    }

    // Show another simple window
    if (show_another_window) {
      ImGui::Begin(
          "Another Window",
          &show_another_window);  // Pass a pointer to our bool variable (the window will have a
                                  // closing button that will clear the bool when clicked)
      ImGui::Text("Hello from another window!");
      if (ImGui::Button("Close Me")) {
        show_another_window = false;
      }
      ImGui::End();
    }
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
        .enable_stats()
        .enable_rest_server()  // Creates REST server on default port (27750), which is required for
                               // using Flecs with the explorer.
        .insert_resource<MyResource>(100)
        .import_module<ModuleOverview>()                // Display module registration overview
        .import_module<vivid::window::WindowSystems>()  // Window management (won't create
                                                        // default)
        .import_module<Setup>()                         // Scene initialization (after window)
        // .import_module<WindowSetup>()                   // Create custom window entity first
        .import_module<vivid::render::RenderSystems>()  // WebGPU rendering (deferred to
                                                        // PreUpdate)
        .import_module<vivid::ui::UISystems>()          // ImGui UI
        .import_module<vivid::input::InputSystems>()    // Input processing and camera control
        .import_module<vivid::input::InputVisualDebugSystems>()  // Mouse input debug panel
        .import_module<ImGuiDemo>();                             // ImGui demo
    // .import_module<VIVID::PHYSICS::PhysicsSystems>()  // Physics simulation
  });
}
}  // namespace vivid::app