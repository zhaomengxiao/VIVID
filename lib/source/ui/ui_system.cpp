#include "vivid/ui/ui_system.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_wgpu.h>
#include <imgui_internal.h>
#include <vivid/log/log.h>
#include <vivid/render/render_component.h>
#include <vivid/render/render_systems.h>
#include <vivid/window/window_component.h>
// #include <webgpu/webgpu.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#  include <emscripten/html5.h>
#  if defined(IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
#    include <emscripten/html5_webgpu.h>
#  endif
#endif

// #if defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
// // #  include <webgpu/webgpu_cpp.h>
// #endif

namespace vivid::ui {

void UISystems::initImGuiImpl(const vivid::window::WindowContext& windowContext,
                              const vivid::render::WebGPUContext& webgpuRes) {
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
  ImGui_ImplSDL3_InitForOther(windowContext.window_handle_);
  ImGui_ImplWGPU_InitInfo init_info;
  init_info.Device = webgpuRes.device;
  init_info.NumFramesInFlight = 3;
  init_info.RenderTargetFormat = webgpuRes.surfaceFormat;
  init_info.DepthStencilFormat = webgpuRes.depthFormat;
  ImGui_ImplWGPU_Init(&init_info);
  VividLogger::app_info("ImGui initialized successfully");

  // Load Fonts
  // Try multiple possible paths for the font file
  const float fontSize = 28.0f;  // Set larger font size
  const char* fontPaths[] = {
      "res/fonts/NotoSans-Regular.ttf",            // Relative to executable (most common)
      "lib/res/fonts/NotoSans-Regular.ttf",        // Relative to project root
      "../lib/res/fonts/NotoSans-Regular.ttf",     // From build directory
      "../../lib/res/fonts/NotoSans-Regular.ttf",  // From deeper build directory
  };

  ImFont* font = nullptr;

  for (const char* fontPath : fontPaths) {
    // Check if file exists
    std::ifstream file(fontPath);
    if (file.good()) {
      file.close();
      font = io.Fonts->AddFontFromFileTTF(fontPath, fontSize);
      if (font != nullptr) {
        VividLogger::app_info("Successfully loaded font from: %s (size: %.1f)", fontPath, fontSize);
        break;
      } else {
        VividLogger::app_warn("Failed to load font from: %s (file exists but loading failed)",
                              fontPath);
      }
    }
  }
}

// Process ImGui events system
void UISystems::processImGuiEventImpl(vivid::app::EventQueues& eventQueues) {
  for (auto& event : eventQueues.raw_sdl_events_) {
    ImGui_ImplSDL3_ProcessEvent(&event);
  }
}

void UISystems::newFrameImpl(const flecs::iter& it) {
  ImGui_ImplWGPU_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  // Create a full-screen dock space for organizing viewports
  ImGuiViewport const* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
  window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
                  | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
  window_flags |= ImGuiWindowFlags_NoBackground;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));

  ImGui::Begin("DockSpace", nullptr, window_flags);
  ImGui::PopStyleVar(3);

  // Create the dock space
  ImGuiID const dockspace_id = ImGui::GetID("MyDockSpace");
  ImGui::DockSpace(dockspace_id, ImVec2(0.0F, 0.0F), ImGuiDockNodeFlags_PassthruCentralNode);

  ImGui::End();
}

// Render ImGui draw data inside active render pass
void UISystems::renderUIImpl(vivid::render::WebGPUContext& webgpuRes) {
  if (webgpuRes.renderPass != nullptr) {
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), webgpuRes.renderPass);
  }
}

void UISystems::endFrameImpl(const flecs::iter& it) {
  ImGui::Render();  // Render() will call EndFrame() internally
}

// Shutdown ImGui system
void UISystems::shutDownUIImpl(const flecs::iter& it) {
  std::cout << "Shutting down ImGui..." << std::endl;

  ImGui::DestroyPlatformWindows();
  ImGui_ImplWGPU_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  std::cout << "ImGui shutdown complete" << std::endl;
}

// Display viewport windows in ImGui
void UISystems::displayViewportWindowsImpl(const flecs::iter& it) {
  auto world = it.world();
  auto viewportQuery
      = world.query<vivid::render::CameraComponent, vivid::render::ViewportComponent>();

  viewportQuery.each([&](flecs::entity entity, const vivid::render::CameraComponent& camera,
                         vivid::render::ViewportComponent& viewport) {
    // Get window title
    std::string windowTitle = "Viewport";
    if (const char* name = entity.name(); name && strlen(name) > 0) {
      windowTitle = name;
    }

    // Create dockable viewport window
    // By default, ImGui only allows dragging windows by their title bar
    // The content area does not respond to drag events
    ImGui::Begin(windowTitle.c_str());
    // Get content region start position in absolute coordinates (recommended API)
    ImVec2 contentStartPos = ImGui::GetCursorScreenPos();
    viewport.content_start_pos_x_ = contentStartPos.x;
    viewport.content_start_pos_y_ = contentStartPos.y;
    viewport.is_focused_ = ImGui::IsWindowFocused();
    viewport.is_hovered_ = ImGui::IsWindowHovered();

    // // DEBUG TEXT
    // ImGui::Text("Viewport: %s", windowTitle.c_str());
    // ImGui::Text("IsFocused: %s", viewport.IsFocused ? "Yes" : "No");
    // ImGui::Text("IsHovered: %s", viewport.IsHovered ? "Yes" : "No");

    // Update viewport size if window size changed
    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    float newWidth = std::max(1.0f, std::min(contentSize.x, 4096.0f));
    float newHeight = std::max(1.0f, std::min(contentSize.y, 4096.0f));

    if (newWidth > 0 && newHeight > 0
        && (std::abs(viewport.width_ - newWidth) > 1.0f
            || std::abs(viewport.height_ - newHeight) > 1.0f)) {
      viewport.width_ = newWidth;
      viewport.height_ = newHeight;
      viewport.initialized_ = false;
    }

    // Display texture
    if (viewport.render_texture_view_ && viewport.texture_id_ != 0) {
      ImGui::Image(reinterpret_cast<ImTextureID>(viewport.render_texture_view_),
                   ImVec2(viewport.width_, viewport.height_));
    } else {
      ImGui::Text("Rendering...");
    }

    ImGui::End();
  });
}

}  // namespace vivid::ui
