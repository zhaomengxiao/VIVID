#pragma once

#include <flecs.h>

#include <glm/glm.hpp>

namespace vivid::ui {

// ImGui state component - stores UI demo state
struct ImGuiState {
  bool show_demo_window_ = true;
  bool show_another_window_ = false;
  glm::vec4 clear_color_ = glm::vec4(0.45F, 0.55F, 0.60F, 1.00F);
  float f_ = 0.0F;
  int counter_ = 0;
};

// UI Components Module
struct UIComponents {
  explicit UIComponents(flecs::world& world) {
    // Register module
    world.module<UIComponents>();

    // Register components
    world.component<ImGuiState>();
  }
};

}  // namespace vivid::ui
