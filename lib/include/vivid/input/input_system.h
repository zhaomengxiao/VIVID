#pragma once

#include <flecs.h>

#include <glm/glm.hpp>

#include "camera_controller.h"
#include "vivid/render/render_component.h"

namespace VIVID::INPUT {

// Mouse input resource - stores global mouse input state (singleton)
struct MouseInputResource {
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

// Input Components Module - registers input-related components
struct InputComponents {
  InputComponents(flecs::world& world) {
    // Register module
    world.module<InputComponents>();

    // Register components
    world.component<CameraControllerComponent>();
    world.component<MouseInputResource>();
  }
};

// Input Systems Module - manages input processing and camera control
struct InputSystems {
  InputSystems(flecs::world& world);

private:
  // Static member functions for system implementations
  static void handleMouseInputImpl(MouseInputResource& mouseInput);
  static void controlCameraImpl(CameraControllerComponent& cameraController,
                                const VIVID::RENDER::ViewportComponent& viewport,
                                VIVID::RENDER::TransformComponent& transform,
                                MouseInputResource& mouseInput);
};

}  // namespace VIVID::INPUT