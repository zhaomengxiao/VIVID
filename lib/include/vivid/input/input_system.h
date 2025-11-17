#pragma once

#include <flecs.h>

#include <glm/glm.hpp>

#include "../render/render_component.h"
#include "camera_controller.h"

// Input Components Module - registers input-related components
struct InputComponents {
  InputComponents(flecs::world& world) {
    // Register module
    world.module<InputComponents>();

    // Register components
    world.component<CameraControllerComponent>();
  }
};

// InputSystem class - currently a placeholder
// Input handling is done through SDL3 events in WindowPlugin
class InputSystem {
  // Input system methods would go here if needed
  // Currently handled through SDL3 event processing in window systems
};
