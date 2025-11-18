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

// Handle mouse input system - updates MouseInputComponent singleton
void UISystems::handleMouseInputImpl(const flecs::iter& it) {
  auto world = it.world();

  // Initialize singleton if it doesn't exist
  if (!world.has<VIVID::UI::MouseInputComponent>()) {
    VividLogger::app_error("MouseInputComponent not found");
    return;
  }

  // Get MouseInputComponent singleton (returns reference)
  MouseInputComponent& mouseInput = world.get_mut<VIVID::UI::MouseInputComponent>();

  // Get ImGui IO for mouse input
  ImGuiIO& io = ImGui::GetIO();

  // Get current mouse position from ImGui
  ImVec2 currentMousePos = ImGui::GetMousePos();
  mouseInput.MousePos = glm::vec2(currentMousePos.x, currentMousePos.y);

  // Use ImGui's built-in mouse delta (already calculated)
  mouseInput.MouseDelta = glm::vec2(io.MouseDelta.x, io.MouseDelta.y);

  // Left mouse button events
  mouseInput.MousePressed = ImGui::IsMouseDown(ImGuiMouseButton_Left);
  mouseInput.MouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
  mouseInput.MouseReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
  mouseInput.MouseDoubleClicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
  mouseInput.MouseDragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
  if (mouseInput.MouseDragging) {
    ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
    mouseInput.LeftMouseDragDelta = glm::vec2(dragDelta.x, dragDelta.y);
  } else {
    mouseInput.LeftMouseDragDelta = glm::vec2(0.0f);
  }

  // Middle mouse button events
  mouseInput.MiddleMousePressed = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
  mouseInput.MiddleMouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Middle);
  mouseInput.MiddleMouseReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Middle);
  mouseInput.MiddleMouseDragging = ImGui::IsMouseDragging(ImGuiMouseButton_Middle);
  if (mouseInput.MiddleMouseDragging) {
    ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
    mouseInput.MiddleMouseDragDelta = glm::vec2(dragDelta.x, dragDelta.y);
  } else {
    mouseInput.MiddleMouseDragDelta = glm::vec2(0.0f);
  }

  // Right mouse button events
  mouseInput.RightMousePressed = ImGui::IsMouseDown(ImGuiMouseButton_Right);
  mouseInput.RightMouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
  mouseInput.RightMouseReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Right);
  mouseInput.RightMouseDoubleClicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Right);
  mouseInput.RightMouseDragging = ImGui::IsMouseDragging(ImGuiMouseButton_Right);
  if (mouseInput.RightMouseDragging) {
    ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
    mouseInput.RightMouseDragDelta = glm::vec2(dragDelta.x, dragDelta.y);
  } else {
    mouseInput.RightMouseDragDelta = glm::vec2(0.0f);
  }

  // Mouse wheel events
  mouseInput.MouseWheelDelta = io.MouseWheel;
  mouseInput.MouseWheelH = io.MouseWheelH;
}

// Display viewport windows in ImGui (rendering handled by RenderSystems)
void UISystems::displayViewportWindowsImpl(const flecs::iter& it) {
  auto world = it.world();
  auto viewportQuery = world.query<RENDER::CameraComponent, RENDER::ViewportComponent>();

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
    // Get content region start position in absolute coordinates (recommended API)
    ImVec2 contentStartPos = ImGui::GetCursorScreenPos();
    viewport.contentStartPos_x = contentStartPos.x;
    viewport.contentStartPos_y = contentStartPos.y;
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

// Control camera system - updates CameraControllerComponent based on mouse input and viewport state
void UISystems::controlCameraImpl(const flecs::iter& it) {
  auto world = it.world();

  // Get MouseInputComponent singleton (returns reference)
  if (!world.has<VIVID::UI::MouseInputComponent>()) {
    return;  // Mouse input not available
  }
  MouseInputComponent& mouseInput = world.get_mut<VIVID::UI::MouseInputComponent>();

  // Query entities with CameraControllerComponent and ViewportComponent
  auto cameraQuery = world.query<CameraControllerComponent, RENDER::ViewportComponent,
                                 RENDER::TransformComponent>();

  cameraQuery.each([&](flecs::entity entity, CameraControllerComponent& cameraController,
                       const RENDER::ViewportComponent& viewport,
                       RENDER::TransformComponent& transform) {
    // Only handle mouse input when viewport is focused and hovered
    if (!viewport.IsFocused || !viewport.IsHovered) {
      return;
    }

    // Convert to local viewport coordinates (relative to content area)
    float localMouseX = mouseInput.MousePos.x - viewport.contentStartPos_x;
    float localMouseY = mouseInput.MousePos.y - viewport.contentStartPos_y;

    // Check if mouse is within viewport area
    bool mouseInViewport = (localMouseX >= 0 && localMouseX <= viewport.Width && localMouseY >= 0
                            && localMouseY <= viewport.Height);

    // Handle mouse drag for camera rotation (when left mouse is dragging and in viewport)
    // Use ImGui's drag detection - no need to check threshold manually
    if (mouseInput.MouseDragging && mouseInViewport) {
      glm::vec2 mouseDelta = mouseInput.LeftMouseDragDelta;

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

      // Reset drag delta to get per-frame delta (ImGui will recalculate from current position)
      ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
    }

    // Handle mouse wheel zoom (only when mouse is in viewport and viewport is focused)
    if (mouseInViewport && std::abs(mouseInput.MouseWheelDelta) > 0.001f) {
      // Calculate zoom amount based on wheel delta and zoom speed
      float zoomAmount = mouseInput.MouseWheelDelta * cameraController.ZoomSpeed;

      // Move camera along Front direction for zoom
      glm::vec3 zoomDirection = cameraController.Front * zoomAmount;
      glm::vec3 newPosition = transform.Position + zoomDirection;

      // Calculate distance from origin to limit zoom range
      // For a simple implementation, we use distance from origin as zoom distance
      float currentDistance = glm::length(transform.Position);
      float newDistance = glm::length(newPosition);

      // Apply zoom limits
      if (newDistance >= cameraController.MinZoom && newDistance <= cameraController.MaxZoom) {
        transform.Position = newPosition;
      } else {
        // Clamp to zoom limits
        glm::vec3 direction = currentDistance > 0.001f ? glm::normalize(transform.Position)
                                                       : glm::vec3(0.0f, 0.0f, -1.0f);
        if (newDistance < cameraController.MinZoom) {
          transform.Position = direction * cameraController.MinZoom;
        } else if (newDistance > cameraController.MaxZoom) {
          transform.Position = direction * cameraController.MaxZoom;
        }
      }
    }

    // Handle middle mouse drag for camera panning
    // Use ImGui's drag detection - no need to check threshold manually
    if (mouseInput.MiddleMouseDragging && mouseInViewport) {
      glm::vec2 mouseDelta = mouseInput.MiddleMouseDragDelta;

      // Calculate pan amount using Right and Up vectors
      float panX
          = mouseDelta.x * cameraController.PanSpeed * -1.0f;  // Negative for natural panning
      float panY = mouseDelta.y * cameraController.PanSpeed;

      // Update camera position using Right and Up vectors
      transform.Position += cameraController.Right * panX + cameraController.Up * panY;

      // Reset drag delta to get per-frame delta (ImGui will recalculate from current position)
      ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
    }
  });
}

}  // namespace UI
}  // namespace VIVID
