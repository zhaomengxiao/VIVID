// SDL3 Hello World Example
// This example demonstrates how to use the new SDL3 callback-based application system

#include <utility>

#include "vivid/app/SDL3App.h"
#include "vivid/log/log.h"
#include "vivid/window/window_systems.h"

// 如何在SDL3窗口中显示imgui: 1.添加imgui头文件
#include <SDL3/SDL_opengl.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include "vivid/render/render_systems.h"

struct MyResource {
  int value;
  MyResource(int v) : value(v) {}
};

// 如何在SDL3窗口中显示imgui: 2.初始化
void initImGui(Resources& res, entt::registry& world) {
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // IF using Docking Branch

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  // Setup scaling
  ImGuiStyle& style = ImGui::GetStyle();
  // style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution
  // for dynamic style scaling, changing this requires resetting Style + calling this again)
  // style.FontScaleDpi = main_scale;        // Set initial font scale. (using
  // io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation
  // purpose) Setup Platform/Renderer backends
  auto view = world.view<VIVID::Window::WindowGpuComponent>();

  view.each([&](auto entity, auto& gpu_comp) {
    ImGui_ImplSDL3_InitForOpenGL(gpu_comp.window_handle, gpu_comp.gl_context);
    ImGui_ImplOpenGL3_Init(nullptr);
    return;  // 只处理第一个窗口
  });

  // Load Fonts
  // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple
  // fonts and use ImGui::PushFont()/PopFont() to select them.
  // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the
  // font among multiple.
  // - If the file cannot be loaded, the function will return a nullptr. Please handle those
  // errors in your application (e.g. use an assertion, or display an error and quit).
  // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher
  // quality font rendering.
  // - Read 'docs/FONTS.md' for more instructions and details. If you like the default font but
  // want it to scale better, consider using the 'ProggyVector' from the same author!
  // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to
  // write a double backslash \\ !
  // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the
  // "fonts/" folder. See Makefile.emscripten for details.
  // style.FontSizeBase = 20.0f;
  // io.Fonts->AddFontDefault();
  // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
  // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
  // IM_ASSERT(font != nullptr);
}

// 如何在SDL3窗口中显示imgui: 3.处理事件
void ProcessImGuiEvent(Resources& res, entt::registry& world) {
  auto eventQueues = res.get<EventQueues>();
  if (eventQueues) {
    ImGui_ImplSDL3_ProcessEvent(&eventQueues->raw_sdl_events.front());
    eventQueues->raw_sdl_events.pop();
  }
}

// 如何在SDL3窗口中显示imgui: 4.显示imgui Demo
void ShowImGuiDemo(Resources& res, entt::registry& world) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  ImGui::ShowDemoWindow();

  // Rendering
  ImGui::Render();
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
  glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
               clear_color.z * clear_color.w, clear_color.w);
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  auto view = world.view<VIVID::Window::WindowGpuComponent>();

  view.each([&](auto entity, auto& gpu_comp) {
    SDL_GL_SwapWindow(gpu_comp.window_handle);
    return;  // 只处理第一个窗口
  });
}

void ShutDownImGui(Resources& res, entt::registry& world) {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
}

// Simple startup system
void hello_startup_system(Resources&, entt::registry& world) {
  VividLogger::app_info("Hello from SDL3 startup system!");

  // Create a simple entity
  auto entity = world.create();
  // Add components as needed...
}

// Custom window creation system - demonstrates ECS approach
void create_custom_window_system(Resources&, entt::registry& world) {
  VividLogger::app_info("Creating custom window entity with ECS components");

  // Create a custom window entity with specific configuration
  auto window_entity = world.create();

  // Configure window component with custom settings
  auto& window_comp = world.emplace<VIVID::Window::WindowComponent>(window_entity);
  window_comp.title = "VIVID Hello SDL3 with ECS Window";
  window_comp.width = 1024;
  window_comp.height = 768;
  window_comp.flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
  window_comp.visible = true;

  VividLogger::app_info("Custom window entity created with title: %s", window_comp.title.c_str());
}

// Simple update system
void hello_update_system(Resources& res, entt::registry& world) {
  static int frame_count = 0;
  frame_count++;

  if (frame_count % 60 == 0) {  // Print every 60 frames
    VividLogger::app_info("SDL3 ECS app running... Frame: %d", frame_count);
    VividLogger::app_debug("MyResource value: %d", res.get<MyResource>()->value);

    // Check window status using ECS approach
    auto window_view
        = world.view<VIVID::Window::WindowComponent, VIVID::Window::WindowGpuComponent>();
    for (auto entity : window_view) {
      auto& window_comp = world.get<VIVID::Window::WindowComponent>(entity);
      auto& gpu_comp = world.get<VIVID::Window::WindowGpuComponent>(entity);

      if (gpu_comp.initialized) {
        VividLogger::app_info("ECS Window '%s' is active and running (%dx%d)",
                              window_comp.title.c_str(), window_comp.width, window_comp.height);

        // Check for window events
        if (world.all_of<VIVID::Window::WindowEventsComponent>(entity)) {
          auto& events_comp = world.get<VIVID::Window::WindowEventsComponent>(entity);
          if (events_comp.quit_requested || events_comp.close_requested) {
            VividLogger::app_info("Window close/quit requested - application should exit");
          }
        }
      }
    }
  }

  // Exit after 300 frames (about 5 seconds at 60 FPS)
  // if (frame_count > 300) {
  //   VividLogger::app_info("Exiting SDL3 application...");
  // }
}

#define MAIN_HELLO_WEBGPU

// Method 1: Using macro with chain calls (most concise)
// VIVID_SDL3_MAIN(.add_plugin<DefaultPlugin>()
//                     .add_startup_system(hello_startup_system)
//                     .add_system(ScheduleLabel::Update, hello_update_system))

// 其他链式调用方法示例：

// Hello SDL3
// Method 2: Direct function with chain calls and metadata (新的简化API)

#ifdef MAIN_HELLO_SDL3
SDL3AppBuilder create_app_instance() {
  return std::move(
      create_sdl3_app()
          // 设置应用基本信息（推荐方法）
          .set_app_info("VIVID Hello SDL3 with ECS Window", "1.0.0", "com.vivid.hello_sdl3")
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
          // .add_plugin<DefaultPlugin>()
          .add_plugin<VIVID::Window::WindowPlugin>()
          .add_startup_system(hello_startup_system)
          .add_startup_system(create_custom_window_system)
          // .add_startup_system(VIVID::Render::test_webgpu_instance)
          .add_system(ScheduleLabel::Startup, initImGui)
          .add_system(ScheduleLabel::Update, ShowImGuiDemo)
          // .add_system(ScheduleLabel::Update, hello_update_system)
          .add_system(ScheduleLabel::Event, ProcessImGuiEvent)
          .add_system(ScheduleLabel::Shutdown, ShutDownImGui));
}
#endif

// Hello WebGPU
#ifdef MAIN_HELLO_WEBGPU
VIVID_SDL3_MAIN(
        .set_app_info("VIVID Hello SDL3 with ECS Window", "1.0.0", "com.vivid.hello_sdl3")
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
        // .add_plugin<DefaultPlugin>()
        .add_plugin<VIVID::Window::WindowPlugin>()
        .add_startup_system(hello_startup_system)
        .add_startup_system(create_custom_window_system)
        .add_startup_system(VIVID::Render::CreateWebGPUInstance)
        .add_startup_system(VIVID::Render::RequestWebGPUAdapterSync)
        .add_startup_system(VIVID::Render::InspectWebGPUAdapter)
    // .add_system(ScheduleLabel::Startup, initImGui)
    // .add_system(ScheduleLabel::Update, ShowImGuiDemo)
    // .add_system(ScheduleLabel::Update, hello_update_system)
    // .add_system(ScheduleLabel::Event, ProcessImGuiEvent)
    // .add_system(ScheduleLabel::Shutdown, ShutDownImGui))
)
#endif

// Method 3: Step-by-step building
// SDL3AppBuilder create_app_instance() {
//   auto builder = create_sdl3_app();
//   builder.add_plugin<DefaultPlugin>();
//   builder.add_startup_system(hello_startup_system);
//   builder.add_system(ScheduleLabel::Update, hello_update_system);
//   builder.insert_resource<MyResource>(100);
//   return std::move(builder);
// }

// Method 4: Using VIVID_SDL3_MAIN macro with simplified metadata API (游戏示例)
// VIVID_SDL3_MAIN(.set_app_info("My Awesome Game", "2.0.0", "com.mygame.app")
//                     .set_metadata(SDL3MetadataProperty::Type, SDL3AppType::Game)
//                     .set_default_log_level(VividLogLevel::Info)
//                     .set_log_level(VividLogCategory::Application, VividLogLevel::Debug)
//                     .insert_resource<MyResource>(100)
//                     .add_plugin<DefaultPlugin>()
//                     .add_startup_system(hello_startup_system)
//                     .add_system(ScheduleLabel::Update, hello_update_system))

// Method 5: Step-by-step with simplified metadata API
// SDL3AppBuilder create_app_instance() {
//   auto builder = create_sdl3_app();
//
//   // Set metadata using simplified API
//   builder.set_app_info("My SDL3 Application", "1.0.0-beta", "com.company.myapp");
//   builder.set_metadata(SDL3MetadataProperty::Creator, "My Company");
//   builder.set_metadata(SDL3MetadataProperty::Type, "application");
//   builder.set_custom_metadata("build_config", "debug");
//
//   // Configure logging
//   builder.set_default_log_level(VividLogLevel::Info);
//   builder.set_log_level(VividLogCategory::Application, VividLogLevel::Debug);
//
//   // Configure app
//   builder.add_plugin<DefaultPlugin>();
//   builder.add_startup_system(hello_startup_system);
//   builder.add_system(ScheduleLabel::Update, hello_update_system);
//   builder.insert_resource<MyResource>(100);
//
//   return std::move(builder);
// }
