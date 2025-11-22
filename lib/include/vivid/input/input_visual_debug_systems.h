#pragma once

#include <flecs.h>

#include "input_component.h"

namespace VIVID::INPUT {

// Input Visual Debug Systems Module - displays mouse input debug information
struct InputVisualDebugSystems {
  InputVisualDebugSystems(flecs::world& world);

private:
  // Static member function for system implementation
  static void displayMouseInputDebugImpl(MouseInputResource& mouseInput);
};

}  // namespace VIVID::INPUT
