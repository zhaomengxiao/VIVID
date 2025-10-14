#include "vivid/ui/ui_system.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_wgpu.h>
#include <vivid/log/log.h>
#include <vivid/render/render_systems.h>
#include <vivid/window/window_component.h>
#include <webgpu/webgpu.h>

#include <iostream>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#  include <emscripten/html5.h>
#  if defined(IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
#    include <emscripten/html5_webgpu.h>
#  endif
#endif

#if defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
#  include <webgpu/webgpu_cpp.h>
#endif

#include "vivid/app/SDL3App.h"

namespace VIVID {
namespace UI {

// Static member function implementations

// Initialize ImGui system
void UISystems::initImGuiImpl(flecs::iter& it) {
  auto world = it.world();

  // Check if ImGui context already exists (runs in PreUpdate, so runs every frame)
  if (ImGui::GetCurrentContext() != nullptr) {
    return;  // Already initialized, skip
  }

  VividLogger::app_debug("Initializing ImGui...");

  // Get WebGPU resources from world singleton
  if (!world.has<RENDER::WebGPUResources>()) {
    VividLogger::app_error("Could not get WebGPU resources!");
    return;
  }
  auto& webgpuRes = world.get<RENDER::WebGPUResources>();

  // Check if WebGPU is initialized
  if (webgpuRes.device == nullptr) {
    VividLogger::app_debug("WebGPU not yet initialized, deferring ImGui init...");
    return;  // Wait for WebGPU to initialize
  }

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // IF using Docking Branch
#ifdef __EMSCRIPTEN__
  io.IniFilename = nullptr;
#endif

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  // Setup scaling
  ImGuiStyle& style = ImGui::GetStyle();
  // style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a
  // solution for dynamic style scaling, changing this requires resetting Style + calling this
  // again) style.FontScaleDpi = main_scale;        // Set initial font scale. (using
  // io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation
  // purpose) Setup Platform/Renderer backends

  // Setup Platform/Renderer backends
  // Query for window entities with WindowGpuComponent
  auto query = world.query<WINDOW::WindowGpuComponent>();

  bool initialized = false;  // TODO: 这里都能移到resource中
  query.each([&](flecs::entity e, WINDOW::WindowGpuComponent& gpu_comp) {
    if (!initialized) {
      VividLogger::app_debug("Initializing ImGui backends for window handle: %p",
                             gpu_comp.window_handle);
      ImGui_ImplSDL3_InitForOther(gpu_comp.window_handle);
      ImGui_ImplWGPU_InitInfo init_info;
      init_info.Device = webgpuRes.device;
      init_info.NumFramesInFlight = 3;
      init_info.RenderTargetFormat = webgpuRes.surfaceFormat;
      init_info.DepthStencilFormat = webgpuRes.depthFormat;
      ImGui_ImplWGPU_Init(&init_info);
      initialized = true;
      VividLogger::app_info("ImGui initialized successfully");
    }
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

// Process ImGui events system
void UISystems::processImGuiEventImpl(flecs::iter& it) {
  auto world = it.world();

  // Get event queues from world singleton
  if (world.has<VIVID::APP::EventQueues>()) {
    auto& eventQueues = world.get_mut<VIVID::APP::EventQueues>();
    if (!eventQueues.raw_sdl_events.empty()) {
      VividLogger::app_info("Processing ImGui event: %d", eventQueues.raw_sdl_events.front().type);
      ImGui_ImplSDL3_ProcessEvent(&eventQueues.raw_sdl_events.front());
      eventQueues.raw_sdl_events
          .pop();  // Note:Maybe Don't pop here, let the window system handle it
    } else {
      VividLogger::app_info("No ImGui event to process!");
    }
  } else {
    VividLogger::app_error("Could not get EventQueues!");
  }
}

// Show ImGui demo system
void UISystems::showImGuiDemoImpl(flecs::iter& it) {
  // Build ImGui frame only; actual rendering happens in Render::Draw
  ImGui_ImplWGPU_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

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

  ImGui::Text("This is some useful text.");  // Display some text (you can use a format strings too)
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

  // Do not call ImGui::Render() here; it will be invoked in Render::Draw
}

// Shutdown ImGui system
void UISystems::shutDownImGuiImpl(flecs::iter& it) {
  std::cout << "Shutting down ImGui..." << std::endl;

  ImGui::DestroyPlatformWindows();
  ImGui_ImplWGPU_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  std::cout << "ImGui shutdown complete" << std::endl;
}

}  // namespace UI
}  // namespace VIVID
