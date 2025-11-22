#pragma once

#include <flecs.h>

#include "camera_controller.h"
#include "input_component.h"
#include "vivid/render/render_component.h"

namespace VIVID::INPUT {

// Input Systems Module - manages input processing and camera control
struct InputSystems {
  explicit InputSystems(flecs::world& world);

private:
  // Static member functions for system implementations
  static void handleMouseInputImpl(MouseInputResource& mouseInput);
  static void controlCameraImpl(CameraControllerComponent& cameraController,
                                const VIVID::RENDER::ViewportComponent& viewport,
                                VIVID::RENDER::TransformComponent& transform,
                                MouseInputResource& mouseInput);
};

}  // namespace VIVID::INPUT