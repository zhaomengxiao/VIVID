// SDL3 Hello World Example
// This example demonstrates how to use the new SDL3 callback-based application system

#include <algorithm>
#include <string>

#include "imgui.h"
#include "vivid/app/SDL3App.h"
#include "vivid/input/camera_controller.h"
#include "vivid/input/input_system.h"
#include "vivid/log/log.h"
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
void print_value(const flecs::world& world, const flecs::cursor& cur) {
  // Get unit entity and component
  flecs::entity u = cur.get_unit();
  const flecs::Unit& u_data = u.get<flecs::Unit>();

  // 获取成员实体以访问元数据（如 Range）
  flecs::entity member = world.entity(ecs_meta_get_member_id(&cur.cursor_));
  if (member.is_valid()) {
    if (member.has<flecs::MemberRanges>()) {
      const flecs::MemberRanges& range = member.get<flecs::MemberRanges>();
      ImGui::Text("Range: %f - %f", range.value.min, range.value.max);
    } else {
      ImGui::Text("No Range");
    }

  } else {
    ImGui::Text("Invalid Member");
  }

  flecs::entity rgbUnit = world.entity<flecs::units::color::Rgb>();

  // Print value with unit symbol
  //   std::cout << cur.get_member() << ": " << cur.get_float() << " "
  //             << (u_data.symbol ? u_data.symbol : "") << "\n";
  char* symbol = nullptr;
  if (u == rgbUnit) {
    symbol = const_cast<char*>("RGB");
  } else {
    symbol = u_data.symbol;
  }

  ImGui::Separator();

  ImGui::Text("%s", cur.get_member().c_str());
  ImGui::Text("%f", cur.get_float());
  ImGui::Text("%s", symbol);
}
// Scene initialization module - creates cube, light, and camera entities
struct Setup {
  Setup(flecs::world& world) {
    VIVID_LOG_SYSTEM("Registering Setup module...");

    using namespace VIVID::RENDER;

    // Register module
    world.module<Setup>();

    // Import required components
    world.import <RenderComponents>();
    world.import <VIVID::INPUT::InputComponents>();

    // Register scene initialization system (runs at startup)
    world.system("SceneInitialization").kind(flecs::OnStart).run(sceneInitializationImpl);

    VIVID_LOG_SUCCESS("Setup module registration completed!");

    // Moved from SceneInitialization system to guarantee immediate component addition
    auto testEntity = world.entity("TestEntity");
    testEntity.ensure<VIVID::RENDER::Color3f>();
    VividLogger::app_info("TestEntity: %s", world.to_json(&testEntity).c_str());
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

    // Calculate initial Yaw and Pitch from Front vector to synchronize with mouse controls
    // Pitch = asin(front.y), Yaw = atan2(front.z, front.x)
    sideCameraController.Pitch = glm::degrees(asin(sideCameraController.Front.y));
    sideCameraController.Yaw
        = glm::degrees(atan2(sideCameraController.Front.z, sideCameraController.Front.x));

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

struct ImGuiDemo {
  ImGuiDemo(flecs::world& world) {
    world.module<ImGuiDemo>();

    // get draw frame phase
    flecs::entity DrawFramePhase = world.lookup("VIVID::UI::UISystems::DrawFramePhase");
    if (DrawFramePhase.id() == 0) {
      VividLogger::error(
          "DrawFramePhase not found! Make sure UISystems is imported before ImGuiDemo.");
      return;
    }

    world.system("ImGuiDemo").kind(DrawFramePhase).run(ImGuiDemoImpl);
  }

private:
  static void ImGuiDemoImpl(flecs::iter& it) {
    // Static state for demo windows
    static bool show_demo_window = true;
    static bool show_another_window = false;
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named
    // window.

    // Our state

    static float f = 0.0f;
    static int counter = 0;

    ImGui::Begin("Hello, world!");  // Create a window called "Hello, world!" and append into it.

    // get testEntity
    flecs::entity testEntity = it.world().lookup("Setup::TestEntity");

    // Use cursor API to print values with units
    // Create cursor for the component
    VIVID::RENDER::Color3f& colorData = testEntity.ensure<VIVID::RENDER::Color3f>();
    flecs::cursor cur = it.world().cursor<VIVID::RENDER::Color3f>(&colorData);
    cur.push();
    print_value(it.world(), cur);
    cur.next();
    print_value(it.world(), cur);
    cur.next();
    print_value(it.world(), cur);
    cur.pop();
    std::string json = std::string(it.world().to_json(&colorData).c_str());
    ImGui::Text("%s", json.c_str());

    ImGui::Separator();
    // Serialize world to JSON
    static char worldJsonBuffer[65536] = "";  // 64KB buffer for JSON text
    std::string worldJson = std::string(it.world().to_json().c_str());

    // Format JSON with basic indentation for better readability
    std::string formattedJson;
    int indentLevel = 0;
    const std::string indentStr = "  ";  // 2 spaces per indent level
    bool inString = false;
    bool escapeNext = false;

    for (size_t i = 0; i < worldJson.size(); ++i) {
      char c = worldJson[i];

      if (escapeNext) {
        formattedJson += c;
        escapeNext = false;
        continue;
      }

      if (c == '\\') {
        escapeNext = true;
        formattedJson += c;
        continue;
      }

      if (c == '"') {
        inString = !inString;
        formattedJson += c;
        continue;
      }

      if (inString) {
        formattedJson += c;
        continue;
      }

      // Format based on JSON structure
      if (c == '{' || c == '[') {
        formattedJson += c;
        formattedJson += '\n';
        indentLevel++;
        for (int j = 0; j < indentLevel; ++j) {
          formattedJson += indentStr;
        }
      } else if (c == '}' || c == ']') {
        formattedJson += '\n';
        indentLevel--;
        for (int j = 0; j < indentLevel; ++j) {
          formattedJson += indentStr;
        }
        formattedJson += c;
      } else if (c == ',') {
        formattedJson += c;
        formattedJson += '\n';
        for (int j = 0; j < indentLevel; ++j) {
          formattedJson += indentStr;
        }
      } else if (c == ':') {
        formattedJson += c;
        formattedJson += ' ';
      } else if (c == ' ' || c == '\n' || c == '\t') {
        // Skip whitespace outside strings
        continue;
      } else {
        formattedJson += c;
      }
    }

    // Copy formatted JSON to buffer
    size_t jsonSize = formattedJson.size();
    size_t copySize = std::min(jsonSize, sizeof(worldJsonBuffer) - 1);
    formattedJson.copy(worldJsonBuffer, copySize);
    worldJsonBuffer[copySize] = '\0';
    if (jsonSize >= sizeof(worldJsonBuffer) - 1) {
      worldJsonBuffer[sizeof(worldJsonBuffer) - 4] = '.';
      worldJsonBuffer[sizeof(worldJsonBuffer) - 3] = '.';
      worldJsonBuffer[sizeof(worldJsonBuffer) - 2] = '.';
    }

    ImVec2 textSize = ImGui::GetContentRegionAvail();
    textSize.y = ImGui::GetTextLineHeight() * 20;  // Set height to 20 lines
    ImGui::InputTextMultiline("##WorldJson", worldJsonBuffer, sizeof(worldJsonBuffer), textSize,
                              ImGuiInputTextFlags_ReadOnly);

    ImGui::Text(
        "This is some useful text.");  // Display some text (you can use a format strings too)
    ImGui::Checkbox("Demo Window",
                    &show_demo_window);  // Edit bools storing our window open/close state
    ImGui::Checkbox("Another Window", &show_another_window);

    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);             // Edit 1 float using a slider
    ImGui::ColorEdit3("clear color", (float*)&clear_color);  // Edit 3 floats representing a color

    if (ImGui::Button("Button"))  // Buttons return true when clicked (most widgets return true
                                  // when edited/activated)
      counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
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
      if (ImGui::Button("Close Me")) show_another_window = false;
      ImGui::End();
    }
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

        .enable_stats()
        .enable_rest_server()  // Creates REST server on default port (27750), which is required for
                               // using Flecs with the explorer.
        .insert_resource<MyResource>(100)
        .import_module<ModuleOverview>()                // Display module registration overview
        .import_module<VIVID::WINDOW::WindowSystems>()  // Window management (won't create
                                                        // default)
        .import_module<Setup>()                         // Scene initialization (after window)
        // .import_module<WindowSetup>()                   // Create custom window entity first
        .import_module<VIVID::RENDER::RenderSystems>()  // WebGPU rendering (deferred to
                                                        // PreUpdate)
        .import_module<VIVID::UI::UISystems>()          // ImGui UI
        .import_module<VIVID::INPUT::InputSystems>()    // Input processing and camera control
        .import_module<ImGuiDemo>()                     // ImGui demo
    // .import_module<VIVID::PHYSICS::PhysicsSystems>()  // Physics simulation

)