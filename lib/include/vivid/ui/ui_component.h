#pragma once

#include <flecs.h>

#include <glm/glm.hpp>

namespace VIVID {
namespace UI {

// ImGui state component - stores UI demo state
struct ImGuiState {
  bool show_demo_window = true;
  bool show_another_window = false;
  float clear_color[4] = {0.45f, 0.55f, 0.60f, 1.00f};
  float f = 0.0f;
  int counter = 0;
};

// Mouse input component - stores global mouse input state (singleton)
struct MouseInputComponent {
  glm::vec2 MousePos = glm::vec2(0.0f);    // Current mouse position (global coordinates)
  glm::vec2 MouseDelta = glm::vec2(0.0f);  // Mouse movement delta (from ImGui)

  // Left mouse button
  bool MousePressed = false;        // Left mouse button is pressed
  bool MouseClicked = false;        // Left mouse button was clicked this frame
  bool MouseReleased = false;       // Left mouse button was released this frame
  bool MouseDoubleClicked = false;  // Left mouse button was double-clicked this frame
  bool MouseDragging = false;       // Left mouse button is dragging (past threshold)
  glm::vec2 LeftMouseDragDelta = glm::vec2(0.0f);  // Left mouse drag delta from click position

  // Middle mouse button
  bool MiddleMousePressed = false;   // Middle mouse button is pressed
  bool MiddleMouseClicked = false;   // Middle mouse button was clicked this frame
  bool MiddleMouseReleased = false;  // Middle mouse button was released this frame
  bool MiddleMouseDragging = false;  // Middle mouse button is dragging (past threshold)
  glm::vec2 MiddleMouseDragDelta = glm::vec2(0.0f);  // Middle mouse drag delta from click position

  // Right mouse button
  bool RightMousePressed = false;        // Right mouse button is pressed
  bool RightMouseClicked = false;        // Right mouse button was clicked this frame
  bool RightMouseReleased = false;       // Right mouse button was released this frame
  bool RightMouseDoubleClicked = false;  // Right mouse button was double-clicked this frame
  bool RightMouseDragging = false;       // Right mouse button is dragging (past threshold)
  glm::vec2 RightMouseDragDelta = glm::vec2(0.0f);  // Right mouse drag delta from click position

  // Mouse wheel
  float MouseWheelDelta = 0.0f;  // Mouse wheel scroll delta this frame (vertical)
  float MouseWheelH = 0.0f;      // Mouse wheel scroll delta this frame (horizontal)
};

// UI Components Module
struct UIComponents {
  UIComponents(flecs::world& world) {
    // Register module
    world.module<UIComponents>();

    // Register components
    world.component<ImGuiState>();
    world.component<MouseInputComponent>();
  }
};

}  // namespace UI
}  // namespace VIVID
