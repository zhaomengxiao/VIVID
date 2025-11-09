#include "vivid/ui/ui_system.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_wgpu.h>
#include <vivid/input/camera_controller.h>
#include <vivid/log/log.h>
#include <vivid/render/render_component.h>
#include <vivid/render/render_systems.h>
#include <vivid/window/window_component.h>
#include <webgpu/webgpu.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <deque>
#include <glm/gtc/matrix_transform.hpp>
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

namespace VIVID {
namespace UI {

// Helper function to convert string to WGPUStringView
static WGPUStringView toWgpuStringView(const char* cString) { return {cString, WGPU_STRLEN}; }

// Static member function implementations

// Initialize ImGui system
void UISystems::initImGuiImpl(const WINDOW::WindowContext& windowContext,
                              const RENDER::WebGPUContext& webgpuRes) {
  VividLogger::app_debug("=== InitImGui system called ===");

  // Check if ImGui context already exists (runs in PreUpdate, so runs every frame)
  if (ImGui::GetCurrentContext() != nullptr) {
    VividLogger::app_warn("ImGui already initialized, skipping...");
    return;  // Already initialized, skip
  }

  // Check if WebGPU is initialized
  if (webgpuRes.device == nullptr) {
    VividLogger::app_error("WebGPU not yet initialized, import RenderSystems first!");
    return;
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
  ImGui_ImplSDL3_InitForOther(windowContext.window_handle);
  ImGui_ImplWGPU_InitInfo init_info;
  init_info.Device = webgpuRes.device;
  init_info.NumFramesInFlight = 3;
  init_info.RenderTargetFormat = webgpuRes.surfaceFormat;
  init_info.DepthStencilFormat = webgpuRes.depthFormat;
  ImGui_ImplWGPU_Init(&init_info);
  VividLogger::app_info("ImGui initialized successfully");

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
void UISystems::processImGuiEventImpl(VIVID::APP::EventQueues& eventQueues) {
  for (auto& event : eventQueues.raw_sdl_events) {
    ImGui_ImplSDL3_ProcessEvent(&event);
  }
}

// Show ImGui demo system
void UISystems::showImGuiDemoImpl(const flecs::iter& it) {
  // Build ImGui frame only; actual rendering happens in Render::Draw
  ImGui_ImplWGPU_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  // // Static state for demo windows
  // static bool show_demo_window = true;
  // static bool show_another_window = false;
  // static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  // // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named
  // // window.

  // // Our state

  // static float f = 0.0f;
  // static int counter = 0;

  // ImGui::Begin("Hello, world!");  // Create a window called "Hello, world!" and append into it.

  // ImGui::Text("This is some useful text.");  // Display some text (you can use a format strings
  // too) ImGui::Checkbox("Demo Window",
  //                 &show_demo_window);  // Edit bools storing our window open/close state
  // ImGui::Checkbox("Another Window", &show_another_window);

  // ImGui::SliderFloat("float", &f, 0.0f, 1.0f);             // Edit 1 float using a slider
  // ImGui::ColorEdit3("clear color", (float*)&clear_color);  // Edit 3 floats representing a color

  // if (ImGui::Button("Button"))  // Buttons return true when clicked (most widgets return true
  //                               // when edited/activated)
  //   counter++;
  // ImGui::SameLine();
  // ImGui::Text("counter = %d", counter);

  // ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
  //             ImGui::GetIO().Framerate);
  // ImGui::End();

  // if (show_demo_window) {
  //   ImGui::ShowDemoWindow();
  // }

  // // Show another simple window
  // if (show_another_window) {
  //   ImGui::Begin(
  //       "Another Window",
  //       &show_another_window);  // Pass a pointer to our bool variable (the window will have a
  //                               // closing button that will clear the bool when clicked)
  //   ImGui::Text("Hello from another window!");
  //   if (ImGui::Button("Close Me")) show_another_window = false;
  //   ImGui::End();
  // }

  // Do not call ImGui::Render() here; it will be invoked in Render::Draw
}

// Render ImGui draw data inside active render pass
void UISystems::renderImGuiImpl(RENDER::WebGPUContext& webgpuRes) {
  // IMPORTANT: This function is responsible for closing the ImGui frame,
  // either by calling ImGui::Render() (which calls EndFrame internally)
  // or by explicitly calling ImGui::EndFrame() if rendering is skipped.

  // If we built a frame but cannot render this tick, close the frame manually
  if (webgpuRes.renderPass == nullptr) {
    ImGui::EndFrame();
    return;
  }

  // Normal rendering: Render() will call EndFrame() internally
  ImGui::Render();
  ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), webgpuRes.renderPass);
}

// Shutdown ImGui system
void UISystems::shutDownImGuiImpl(const flecs::iter& it) {
  std::cout << "Shutting down ImGui..." << std::endl;

  ImGui::DestroyPlatformWindows();
  ImGui_ImplWGPU_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  std::cout << "ImGui shutdown complete" << std::endl;
}

// Display viewport windows in ImGui (rendering handled by RenderSystems)
void UISystems::displayViewportWindowsImpl(const flecs::iter& it) {
  auto world = it.world();
  auto viewportQuery = world.query<RENDER::CameraComponent, RENDER::ViewportComponent>();

  viewportQuery.each([&](flecs::entity entity, const RENDER::CameraComponent& camera,
                         RENDER::ViewportComponent& viewport) {
    // Get window title
    std::string windowTitle = "Viewport";
    if (entity.has<RENDER::TagComponent>()) {
      windowTitle = entity.get<RENDER::TagComponent>().Tag;
    } else if (const char* name = entity.name(); name && strlen(name) > 0) {
      windowTitle = name;
    }

    ImGui::Begin(windowTitle.c_str());
    viewport.IsFocused = ImGui::IsWindowFocused();
    viewport.IsHovered = ImGui::IsWindowHovered();

    // Update viewport size if window size changed
    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    float newWidth = std::max(1.0f, std::min(contentSize.x, 4096.0f));
    float newHeight = std::max(1.0f, std::min(contentSize.y, 4096.0f));

    if (newWidth > 0 && newHeight > 0
        && (std::abs(viewport.Width - newWidth) > 1.0f
            || std::abs(viewport.Height - newHeight) > 1.0f)) {
      viewport.Width = newWidth;
      viewport.Height = newHeight;
      viewport.initialized = false;
    }

    // Display texture
    if (viewport.renderTextureView && viewport.TextureID != 0) {
      ImGui::Image(reinterpret_cast<ImTextureID>(viewport.renderTextureView),
                   ImVec2(viewport.Width, viewport.Height));
    } else {
      ImGui::Text("Rendering...");
    }

    ImGui::End();
  });
}

}  // namespace UI
}  // namespace VIVID
