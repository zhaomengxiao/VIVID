#include "vivid/input/input_system.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <vivid/log/log.h>
#include <vivid/render/render_component.h>

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace vivid::input {

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
      .system<CameraControllerComponent, vivid::render::ViewportComponent,
              vivid::render::TransformComponent, MouseInputResource>("ControlCamera")
      .term_at(3)
      .src<MouseInputResource>()
      .kind(flecs::OnUpdate)
      .each(controlCameraImpl);

  VividLogger::app_info("InputSystems module registration completed!");
}

// Handle mouse input system - updates MouseInputResource singleton
void InputSystems::handleMouseInputImpl(MouseInputResource& mouse_input) {
  // Get ImGui IO for mouse input
  const ImGuiIO& io = ImGui::GetIO();

  // Get current mouse position from ImGui
  const ImVec2 kCurrentMousePos = ImGui::GetMousePos();
  mouse_input.mouse_pos_ = glm::vec2(kCurrentMousePos.x, kCurrentMousePos.y);

  // Use ImGui's built-in mouse delta (already calculated)
  mouse_input.mouse_delta_ = glm::vec2(io.MouseDelta.x, io.MouseDelta.y);

  // Left mouse button events
  mouse_input.mouse_pressed_ = ImGui::IsMouseDown(ImGuiMouseButton_Left);
  mouse_input.mouse_clicked_ = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
  mouse_input.mouse_released_ = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
  mouse_input.mouse_double_clicked_ = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
  mouse_input.mouse_dragging_ = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
  if (mouse_input.mouse_dragging_) {
    const ImVec2 kLeftDragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
    mouse_input.left_mouse_drag_delta_ = glm::vec2(kLeftDragDelta.x, kLeftDragDelta.y);
  } else {
    mouse_input.left_mouse_drag_delta_ = glm::vec2(0.0F);
  }

  // Middle mouse button events
  mouse_input.middle_mouse_pressed_ = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
  mouse_input.middle_mouse_clicked_ = ImGui::IsMouseClicked(ImGuiMouseButton_Middle);
  mouse_input.middle_mouse_released_ = ImGui::IsMouseReleased(ImGuiMouseButton_Middle);
  mouse_input.middle_mouse_dragging_ = ImGui::IsMouseDragging(ImGuiMouseButton_Middle);
  if (mouse_input.middle_mouse_dragging_) {
    const ImVec2 kMiddleDragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
    mouse_input.middle_mouse_drag_delta_ = glm::vec2(kMiddleDragDelta.x, kMiddleDragDelta.y);
  } else {
    mouse_input.middle_mouse_drag_delta_ = glm::vec2(0.0F);
  }

  // Right mouse button events
  mouse_input.right_mouse_pressed_ = ImGui::IsMouseDown(ImGuiMouseButton_Right);
  mouse_input.right_mouse_clicked_ = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
  mouse_input.right_mouse_released_ = ImGui::IsMouseReleased(ImGuiMouseButton_Right);
  mouse_input.right_mouse_double_clicked_ = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Right);
  mouse_input.right_mouse_dragging_ = ImGui::IsMouseDragging(ImGuiMouseButton_Right);
  if (mouse_input.right_mouse_dragging_) {
    const ImVec2 kRightDragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
    mouse_input.right_mouse_drag_delta_ = glm::vec2(kRightDragDelta.x, kRightDragDelta.y);
  } else {
    mouse_input.right_mouse_drag_delta_ = glm::vec2(0.0F);
  }

  // Mouse wheel events
  mouse_input.mouse_wheel_delta_ = io.MouseWheel;
  mouse_input.mouse_wheel_h_ = io.MouseWheelH;
}

// Control camera system - updates CameraControllerComponent based on mouse input and viewport state
void InputSystems::controlCameraImpl(CameraControllerComponent& camera_controller,
                                     const vivid::render::ViewportComponent& viewport,
                                     vivid::render::TransformComponent& transform,
                                     MouseInputResource& mouse_input) {
  // Only handle mouse input when viewport is focused and hovered
  if (!viewport.is_focused_ || !viewport.is_hovered_) {
    return;
  }

  // Convert to local viewport coordinates (relative to content area)
  const float kLocalMouseX = mouse_input.mouse_pos_.x - viewport.content_start_pos_x_;
  const float kLocalMouseY = mouse_input.mouse_pos_.y - viewport.content_start_pos_y_;

  // Check if mouse is within viewport area
  const bool kMouseInViewport = (kLocalMouseX >= 0 && kLocalMouseX <= viewport.width_
                                 && kLocalMouseY >= 0 && kLocalMouseY <= viewport.height_);

  // Handle mouse drag for camera rotation (when left mouse is dragging and in viewport)
  // Use ImGui's drag detection - no need to check threshold manually
  if (mouse_input.mouse_dragging_ && kMouseInViewport) {
    const glm::vec2 kMouseDelta = mouse_input.left_mouse_drag_delta_;

    // Apply mouse sensitivity and update yaw/pitch
    camera_controller.yaw_ += kMouseDelta.x * camera_controller.mouse_sensitivity_;
    camera_controller.pitch_ -= kMouseDelta.y * camera_controller.mouse_sensitivity_;

    // Constrain pitch to prevent camera flipping
    camera_controller.pitch_ = std::min(camera_controller.pitch_, 89.0F);
    camera_controller.pitch_ = std::max(camera_controller.pitch_, -89.0F);

    // Update camera vectors based on new yaw/pitch
    camera_controller.UpdateVectors();

    // Update transform rotation from camera controller
    transform.rotation_.x = camera_controller.pitch_;
    transform.rotation_.y = camera_controller.yaw_;
    transform.rotation_.z = 0.0F;

    // Reset drag delta to get per-frame delta (ImGui will recalculate from current position)
    ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
  }

  // Handle mouse wheel zoom (only when mouse is in viewport and viewport is focused)
  if (kMouseInViewport && std::abs(mouse_input.mouse_wheel_delta_) > 0.001F) {
    // Calculate zoom amount based on wheel delta and zoom speed
    const float kZoomAmount = mouse_input.mouse_wheel_delta_ * camera_controller.zoom_speed_;

    // Move camera along Front direction for zoom
    const glm::vec3 kZoomDirection = camera_controller.front_ * kZoomAmount;
    const glm::vec3 kNewPosition = transform.position_ + kZoomDirection;

    // Calculate distance from origin to limit zoom range
    // For a simple implementation, we use distance from origin as zoom distance
    const float kCurrentDistance = glm::length(transform.position_);
    const float kNewDistance = glm::length(kNewPosition);

    // Apply zoom limits
    if (kNewDistance >= camera_controller.min_zoom_
        && kNewDistance <= camera_controller.max_zoom_) {
      transform.position_ = kNewPosition;
    } else {
      // Clamp to zoom limits
      const glm::vec3 kDirection = kCurrentDistance > 0.001F ? glm::normalize(transform.position_)
                                                             : glm::vec3(0.0F, 0.0F, -1.0F);
      if (kNewDistance < camera_controller.min_zoom_) {
        transform.position_ = kDirection * camera_controller.min_zoom_;
      } else if (kNewDistance > camera_controller.max_zoom_) {
        transform.position_ = kDirection * camera_controller.max_zoom_;
      }
    }
  }

  // Handle middle mouse drag for camera panning
  // Use ImGui's drag detection - no need to check threshold manually
  if (mouse_input.middle_mouse_dragging_ && kMouseInViewport) {
    const glm::vec2 kMouseDelta = mouse_input.middle_mouse_drag_delta_;

    // Calculate pan amount using Right and Up vectors
    const float kPanX
        = kMouseDelta.x * camera_controller.pan_speed_ * -1.0F;  // Negative for natural panning
    const float kPanY = kMouseDelta.y * camera_controller.pan_speed_;

    // Update camera position using Right and Up vectors
    transform.position_ += camera_controller.right_ * kPanX + camera_controller.up_ * kPanY;

    // Reset drag delta to get per-frame delta (ImGui will recalculate from current position)
    ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
  }
}

}  // namespace vivid::input
