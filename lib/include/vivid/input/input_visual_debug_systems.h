#pragma once

#include <flecs.h>

#include "input_component.h"

struct KeyboardInputResource;

namespace vivid::input {

// Input Visual Debug Systems Module - displays mouse input debug information
struct InputVisualDebugSystems {
  explicit InputVisualDebugSystems(flecs::world& world);

private:
  // Static member functions for system implementations
  static void displayMouseInputDebugImpl(const MouseInputResource& mouse_input);
  static void displayKeyboardInputDebugImpl(const KeyboardInputResource& keyboard_input);
};

}  // namespace vivid::input
