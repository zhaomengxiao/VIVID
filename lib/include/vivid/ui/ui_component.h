#pragma once

#include <flecs.h>

#include <glm/glm.hpp>

namespace VIVID::UI {

// ImGui state component - stores UI demo state
struct ImGuiState {
  bool show_demo_window = true;
  bool show_another_window = false;
  glm::vec4 clear_color = glm::vec4(0.45F, 0.55f, 0.60f, 1.00f);
  float f = 0.0f;
  int counter = 0;
};

// UI Components Module
struct UIComponents {
  UIComponents(flecs::world& world) {
    // Register module
    world.module<UIComponents>();

    // Register components
    world.component<ImGuiState>();
  }
};

}  // namespace VIVID::UI
