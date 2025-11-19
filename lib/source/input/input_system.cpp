#include "vivid/input/input_system.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <vivid/log/log.h>
#include <vivid/render/render_component.h>

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace VIVID::INPUT {

// Constructor - Register module and systems
InputSystems::InputSystems(flecs::world& world) {
  VividLogger::app_info("Registering InputSystems...");

  // Register module
  world.module<InputSystems>();

  // Import components module
  world.import <InputComponents>();

  // Initialize MouseInputResource singleton
  world.set<MouseInputResource>({});

  // MouseWheel must be handled after NewFrame to get the correct mouse wheel delta
  world.system<MouseInputResource>("HandleMouseInput")
      .kind(flecs::OnUpdate)
      .each(handleMouseInputImpl);
  world
      .system<CameraControllerComponent, VIVID::RENDER::ViewportComponent,
              VIVID::RENDER::TransformComponent, MouseInputResource>("ControlCamera")
      .term_at(3)
      .src<MouseInputResource>()
      .kind(flecs::OnUpdate)
      .each(controlCameraImpl);

  VividLogger::app_info("InputSystems module registration completed!");
}

// Handle mouse input system - updates MouseInputResource singleton
void InputSystems::handleMouseInputImpl(MouseInputResource& mouseInput) {
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

// Control camera system - updates CameraControllerComponent based on mouse input and viewport state
void InputSystems::controlCameraImpl(CameraControllerComponent& cameraController,
                                     const VIVID::RENDER::ViewportComponent& viewport,
                                     VIVID::RENDER::TransformComponent& transform,
                                     MouseInputResource& mouseInput) {
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
    float panX = mouseDelta.x * cameraController.PanSpeed * -1.0f;  // Negative for natural panning
    float panY = mouseDelta.y * cameraController.PanSpeed;

    // Update camera position using Right and Up vectors
    transform.Position += cameraController.Right * panX + cameraController.Up * panY;

    // Reset drag delta to get per-frame delta (ImGui will recalculate from current position)
    ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
  }
}

}  // namespace VIVID::INPUT
