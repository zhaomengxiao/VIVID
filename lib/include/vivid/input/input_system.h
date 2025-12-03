#pragma once

#include <flecs.h>

#include "camera_controller.h"
#include "input_component.h"
#include "vivid/render/render_component.h"

struct KeyboardInputResource;

namespace vivid::input {

// Input Systems Module - manages input processing and camera control
struct InputSystems {
  explicit InputSystems(flecs::world& world);

private:
  // Static member functions for system implementations
  static void handleMouseInputImpl(MouseInputResource& mouse_input);
  static void handleKeyboardInputImpl(KeyboardInputResource& keyboard_input);
  static void controlCameraImpl(CameraControllerComponent& camera_controller,
                                const vivid::render::ViewportComponent& viewport,
                                vivid::render::TransformComponent& transform,
                                MouseInputResource& mouse_input,
                                KeyboardInputResource& keyboard_input);
};

}  // namespace vivid::input
