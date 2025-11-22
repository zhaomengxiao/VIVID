#pragma once

#include <flecs.h>

#include <glm/glm.hpp>

#include "camera_controller.h"

namespace vivid::input {

// Mouse input resource - stores global mouse input state (singleton)
struct MouseInputResource {
  glm::vec2 mouse_pos_ = glm::vec2(0.0F);    // Current mouse position (global coordinates)
  glm::vec2 mouse_delta_ = glm::vec2(0.0F);  // Mouse movement delta (from ImGui)

  // Left mouse button
  bool mouse_pressed_ = false;         // Left mouse button is pressed
  bool mouse_clicked_ = false;         // Left mouse button was clicked this frame
  bool mouse_released_ = false;        // Left mouse button was released this frame
  bool mouse_double_clicked_ = false;  // Left mouse button was double-clicked this frame
  bool mouse_dragging_ = false;        // Left mouse button is dragging (past threshold)
  glm::vec2 left_mouse_drag_delta_ = glm::vec2(0.0F);  // Left mouse drag delta from click position

  // Middle mouse button
  bool middle_mouse_pressed_ = false;   // Middle mouse button is pressed
  bool middle_mouse_clicked_ = false;   // Middle mouse button was clicked this frame
  bool middle_mouse_released_ = false;  // Middle mouse button was released this frame
  bool middle_mouse_dragging_ = false;  // Middle mouse button is dragging (past threshold)
  glm::vec2 middle_mouse_drag_delta_
      = glm::vec2(0.0F);  // Middle mouse drag delta from click position

  // Right mouse button
  bool right_mouse_pressed_ = false;         // Right mouse button is pressed
  bool right_mouse_clicked_ = false;         // Right mouse button was clicked this frame
  bool right_mouse_released_ = false;        // Right mouse button was released this frame
  bool right_mouse_double_clicked_ = false;  // Right mouse button was double-clicked this frame
  bool right_mouse_dragging_ = false;        // Right mouse button is dragging (past threshold)
  glm::vec2 right_mouse_drag_delta_
      = glm::vec2(0.0F);  // Right mouse drag delta from click position

  // Mouse wheel
  float mouse_wheel_delta_ = 0.0F;  // Mouse wheel scroll delta this frame (vertical)
  float mouse_wheel_h_ = 0.0F;      // Mouse wheel scroll delta this frame (horizontal)
};

// Input Components Module - registers input-related components
struct InputComponents {
  explicit InputComponents(flecs::world& world) {
    // Register module
    world.module<InputComponents>();

    // Register components
    world.component<CameraControllerComponent>();
    world.component<MouseInputResource>();
  }
};

}  // namespace vivid::input
