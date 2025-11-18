#include "vivid/ui/ui_system.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_wgpu.h>
#include <imgui_internal.h>
#include <vivid/input/camera_controller.h>
#include <vivid/log/log.h>
#include <vivid/render/render_component.h>
#include <vivid/render/render_systems.h>
#include <vivid/window/window_component.h>
// #include <webgpu/webgpu.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>

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

namespace VIVID {
namespace UI {

// Tooltip info structure for camera controller debug display
struct TooltipInfo {
  std::string title;
  bool isActive;
  bool isDragging;
  float yaw, pitch;
  float mouseX, mouseY;
  bool mouseInViewport;
  glm::vec3 front, up;
  ImVec2 mousePos;
  bool show = false;
};

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

void UISystems::newFrameImpl(const flecs::iter& it) {
  ImGui_ImplWGPU_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  // Create a full-screen dock space for organizing viewports
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
  window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
                  | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
  window_flags |= ImGuiWindowFlags_NoBackground;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

  ImGui::Begin("DockSpace", nullptr, window_flags);
  ImGui::PopStyleVar(3);

  // Create the dock space
  ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
  ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

  ImGui::End();
}

// Render ImGui draw data inside active render pass
void UISystems::renderUIImpl(RENDER::WebGPUContext& webgpuRes) {
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

// Display viewport windows in ImGui (rendering handled by RenderSystems)
void UISystems::displayViewportWindowsImpl(const flecs::iter& it) {
  auto world = it.world();
  auto viewportQuery = world.query<RENDER::CameraComponent, RENDER::ViewportComponent>();

  // Store tooltip info for all viewports (will be rendered after all viewports)
  static std::vector<TooltipInfo> tooltips;

  // Clear previous tooltips
  tooltips.clear();

  viewportQuery.each([&](flecs::entity entity, const RENDER::CameraComponent& camera,
                         RENDER::ViewportComponent& viewport) {
    // Get window title
    std::string windowTitle = "Viewport";
    if (const char* name = entity.name(); name && strlen(name) > 0) {
      windowTitle = name;
    }

    // Create dockable viewport window
    // By default, ImGui only allows dragging windows by their title bar
    // The content area does not respond to drag events
    ImGui::Begin(windowTitle.c_str());
    viewport.IsFocused = ImGui::IsWindowFocused();
    viewport.IsHovered = ImGui::IsWindowHovered();

    // DEBUG TEXT
    ImGui::Text("Viewport: %s", windowTitle.c_str());
    ImGui::Text("IsFocused: %s", viewport.IsFocused ? "Yes" : "No");
    ImGui::Text("IsHovered: %s", viewport.IsHovered ? "Yes" : "No");

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

    // Camera controller logic for viewport
    if (entity.has<CameraControllerComponent>()) {
      auto& cameraController = entity.get_mut<CameraControllerComponent>();
      auto& transform = entity.get_mut<RENDER::TransformComponent>();

      // Handle mouse input when viewport is focused and hovered
      if (viewport.IsFocused && viewport.IsHovered) {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 contentMin = ImGui::GetWindowContentRegionMin();

        // Convert to local viewport coordinates (relative to content area)
        float localMouseX = mousePos.x - windowPos.x - contentMin.x;
        float localMouseY = mousePos.y - windowPos.y - contentMin.y;

        // Check if mouse is within viewport area
        bool mouseInViewport = (localMouseX >= 0 && localMouseX <= viewport.Width
                                && localMouseY >= 0 && localMouseY <= viewport.Height);

        // Handle mouse button press/release events
        // Check for mouse press (only when mouse is in viewport area)
        if (mouseInViewport && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
          cameraController.MousePressed = true;
          cameraController.IsActive = true;  // Keep camera controller active during drag
          cameraController.LastMousePos = glm::vec2(localMouseX, localMouseY);
          std::cout << "[MOUSE] Started dragging in viewport: " << windowTitle << std::endl;
        }

        // Check for mouse release (anywhere - to stop dragging)
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && cameraController.MousePressed) {
          cameraController.MousePressed = false;
          // Keep IsActive true so camera can still be controlled, but stop mouse tracking
          std::cout << "[MOUSE] Stopped dragging: " << windowTitle << std::endl;
        }

        // Handle mouse drag for camera rotation (only when mouse is pressed, active, and still in
        // viewport)
        if (cameraController.MousePressed && cameraController.IsActive && mouseInViewport) {
          glm::vec2 currentMousePos(localMouseX, localMouseY);
          glm::vec2 mouseDelta = currentMousePos - cameraController.LastMousePos;

          // Only update if there's actual mouse movement
          if (glm::length(mouseDelta) > 0.1f) {  // Small threshold to avoid noise
            // Apply mouse sensitivity and update yaw/pitch
            cameraController.Yaw += mouseDelta.x * cameraController.MouseSensitivity;
            cameraController.Pitch -= mouseDelta.y * cameraController.MouseSensitivity;

            // Constrain pitch to prevent camera flipping
            if (cameraController.Pitch > 89.0f) cameraController.Pitch = 89.0f;
            if (cameraController.Pitch < -89.0f) cameraController.Pitch = -89.0f;

            // Update camera vectors based on new yaw/pitch
            cameraController.UpdateVectors();

            // Update transform rotation from camera controller
            transform.Rotation.x = cameraController.Pitch;
            transform.Rotation.y = cameraController.Yaw;
            transform.Rotation.z = 0.0f;
          }

          // Update last mouse position
          cameraController.LastMousePos = currentMousePos;
        }

        // Store debug info for tooltip display (will be rendered after all viewports)
        if (cameraController.IsActive || cameraController.MousePressed) {
          TooltipInfo tooltip;
          tooltip.title = windowTitle;
          tooltip.isActive = cameraController.IsActive;
          tooltip.isDragging = cameraController.MousePressed;
          tooltip.yaw = cameraController.Yaw;
          tooltip.pitch = cameraController.Pitch;
          tooltip.mouseX = localMouseX;
          tooltip.mouseY = localMouseY;
          tooltip.mouseInViewport = mouseInViewport;
          tooltip.front = cameraController.Front;
          tooltip.up = cameraController.Up;
          tooltip.mousePos = mousePos;
          tooltip.show = true;

          tooltips.push_back(tooltip);
        }
      }
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

  // // Render tooltips for all camera controllers (outside viewport windows)
  // for (const auto& tooltip : tooltips) {
  //   if (tooltip.show) {
  //     ImGui::SetNextWindowPos(ImVec2(tooltip.mousePos.x + 15, tooltip.mousePos.y + 15));
  //     ImGui::SetNextWindowBgAlpha(0.9f);
  //     ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
  //     ImGui::Begin((std::string("##") + tooltip.title + "_tooltip").c_str(), nullptr,
  //                  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
  //                  ImGuiWindowFlags_NoMove
  //                      | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

  //     ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s Camera Controller",
  //                        tooltip.title.c_str());
  //     ImGui::Separator();

  //     ImGui::Text("Active: %s", tooltip.isActive ? "Yes" : "No");
  //     ImGui::Text("Dragging: %s", tooltip.isDragging ? "Yes" : "No");
  //     ImGui::Text("Yaw: %.1f°", tooltip.yaw);
  //     ImGui::Text("Pitch: %.1f°", tooltip.pitch);

  //     if (tooltip.isDragging) {
  //       ImGui::Text("Mouse: (%.0f, %.0f)", tooltip.mouseX, tooltip.mouseY);
  //       ImGui::Text("In Viewport: %s", tooltip.mouseInViewport ? "Yes" : "No");
  //     }

  //     ImGui::Text("Front: (%.2f, %.2f, %.2f)", tooltip.front.x, tooltip.front.y,
  //     tooltip.front.z); ImGui::Text("Up: (%.2f, %.2f, %.2f)", tooltip.up.x, tooltip.up.y,
  //     tooltip.up.z);

  //     ImGui::End();
  //     ImGui::PopStyleVar();

  //     // Bring tooltip to front to ensure it's visible above other windows
  //     if (ImGuiWindow* window
  //         = ImGui::FindWindowByName((std::string("##") + tooltip.title + "_tooltip").c_str())) {
  //       ImGui::BringWindowToDisplayFront(window);
  //     }
  //   }
  // }
}

}  // namespace UI
}  // namespace VIVID
