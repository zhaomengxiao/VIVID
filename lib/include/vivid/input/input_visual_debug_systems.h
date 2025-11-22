#pragma once

#include <flecs.h>

#include "input_component.h"

namespace vivid::input {

// Input Visual Debug Systems Module - displays mouse input debug information
struct InputVisualDebugSystems {
  explicit InputVisualDebugSystems(flecs::world& world);

private:
  // Static member function for system implementation
  static void displayMouseInputDebugImpl(const MouseInputResource& mouse_input);
};

}  // namespace vivid::input
