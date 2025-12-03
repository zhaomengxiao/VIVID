#pragma once

#include <flecs.h>

#include <array>
#include <glm/glm.hpp>
#include <imgui.h>

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

// Keyboard input resource - stores global keyboard input state (singleton)
struct KeyboardInputResource {
  // Key state arrays - indexed by (ImGuiKey - ImGuiKey_NamedKey_BEGIN)
  // Only keys from ImGuiKey_NamedKey_BEGIN to ImGuiKey_NamedKey_END are stored
  static constexpr int kKeyStateArraySize = ImGuiKey_NamedKey_COUNT;

  std::array<bool, kKeyStateArraySize> key_pressed_ = {};     // Key is currently pressed down
  std::array<bool, kKeyStateArraySize> key_down_ = {};        // Key was pressed this frame
  std::array<bool, kKeyStateArraySize> key_released_ = {};    // Key was released this frame

  // Helper functions to access key states safely
  bool IsKeyPressed(ImGuiKey key) const {
    if (key >= ImGuiKey_NamedKey_BEGIN && key < ImGuiKey_NamedKey_END) {
      return key_pressed_[key - ImGuiKey_NamedKey_BEGIN];
    }
    return false;
  }

  bool IsKeyDown(ImGuiKey key) const {
    if (key >= ImGuiKey_NamedKey_BEGIN && key < ImGuiKey_NamedKey_END) {
      return key_down_[key - ImGuiKey_NamedKey_BEGIN];
    }
    return false;
  }

  bool IsKeyReleased(ImGuiKey key) const {
    if (key >= ImGuiKey_NamedKey_BEGIN && key < ImGuiKey_NamedKey_END) {
      return key_released_[key - ImGuiKey_NamedKey_BEGIN];
    }
    return false;
  }

  // Common modifier key helpers
  bool ctrl_pressed() const { return IsKeyPressed(ImGuiKey_LeftCtrl) || IsKeyPressed(ImGuiKey_RightCtrl); }
  bool shift_pressed() const { return IsKeyPressed(ImGuiKey_LeftShift) || IsKeyPressed(ImGuiKey_RightShift); }
  bool alt_pressed() const { return IsKeyPressed(ImGuiKey_LeftAlt) || IsKeyPressed(ImGuiKey_RightAlt); }
  bool super_pressed() const { return IsKeyPressed(ImGuiKey_LeftSuper) || IsKeyPressed(ImGuiKey_RightSuper); }
};

// Input Components Module - registers input-related components
struct InputComponents {
  explicit InputComponents(flecs::world& world) {
    // Register module
    world.module<InputComponents>();

    // Register components
    world.component<CameraControllerComponent>();
    world.component<MouseInputResource>();
    world.component<KeyboardInputResource>();
  }
};

}  // namespace vivid::input
