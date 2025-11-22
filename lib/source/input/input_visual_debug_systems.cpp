#include "vivid/input/input_visual_debug_systems.h"

#include <imgui.h>
#include <vivid/input/input_component.h>
#include <vivid/log/log.h>

#include <string>

namespace vivid::input {

// Color constants for debug display
namespace {
constexpr ImVec4 kGreenBackground(0.0F, 0.5F, 0.0F, 1.0F);  // Dark green background for true values

// Helper function to display bool value with green background when true
void DisplayBoolValue(const char* label, bool value) {
  const std::string kText = "  " + std::string(label) + ": " + (value ? "Yes" : "No");

  // Disable button interaction to make it display-only
  ImGui::BeginDisabled();

  if (value) {
    // Push green background color for the button frame when value is true
    ImGui::PushStyleColor(ImGuiCol_Button, kGreenBackground);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kGreenBackground);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, kGreenBackground);
  }

  // Use Button with full width (-1) to show text with background color that fills the width
  ImGui::Button(kText.c_str(), ImVec2(-1, 0));

  if (value) {
    ImGui::PopStyleColor(3);
  }

  ImGui::EndDisabled();
}
}  // anonymous namespace

// Constructor - Register module and systems
InputVisualDebugSystems::InputVisualDebugSystems(flecs::world& world) {
  VividLogger::app_info("Registering InputVisualDebugSystems...");

  // Register module
  world.module<InputVisualDebugSystems>();

  // Import components module
  world.import <InputComponents>();

  // Register system to display mouse input debug panel
  world.system<MouseInputResource>("DisplayMouseInputDebug")
      .kind(flecs::OnUpdate)
      .term_at(0)
      .src<MouseInputResource>()
      .each(displayMouseInputDebugImpl);

  VividLogger::app_info(
      "InputVisualDebugSystems module registration completed!");  // NOLINT(cppcoreguidelines-pro-type-vararg)
}

// Display mouse input debug panel - shows all MouseInputResource values
void InputVisualDebugSystems::displayMouseInputDebugImpl(const MouseInputResource& mouse_input) {
  if (!ImGui::Begin("Mouse Input Debug")) {
    ImGui::End();
    return;
  }

  // Mouse Position & Delta section
  ImGui::Text("Mouse Position & Delta");
  ImGui::Text("  Position: (%.2f, %.2f)", mouse_input.mouse_pos_.x, mouse_input.mouse_pos_.y);
  ImGui::Text("  Delta: (%.2f, %.2f)", mouse_input.mouse_delta_.x, mouse_input.mouse_delta_.y);

  ImGui::Separator();

  // Left Mouse Button section
  ImGui::Text("Left Mouse Button");
  ImGui::PushID("LeftMouse");
  DisplayBoolValue("Pressed", mouse_input.mouse_pressed_);
  DisplayBoolValue("Clicked", mouse_input.mouse_clicked_);
  DisplayBoolValue("Released", mouse_input.mouse_released_);
  DisplayBoolValue("Double Clicked", mouse_input.mouse_double_clicked_);
  DisplayBoolValue("Dragging", mouse_input.mouse_dragging_);
  ImGui::PopID();
  ImGui::Text("  Drag Delta: (%.2f, %.2f)", mouse_input.left_mouse_drag_delta_.x,
              mouse_input.left_mouse_drag_delta_.y);

  ImGui::Separator();

  // Middle Mouse Button section
  ImGui::Text("Middle Mouse Button");
  ImGui::PushID("MiddleMouse");
  DisplayBoolValue("Pressed", mouse_input.middle_mouse_pressed_);
  DisplayBoolValue("Clicked", mouse_input.middle_mouse_clicked_);
  DisplayBoolValue("Released", mouse_input.middle_mouse_released_);
  DisplayBoolValue("Dragging", mouse_input.middle_mouse_dragging_);
  ImGui::PopID();
  ImGui::Text("  Drag Delta: (%.2f, %.2f)", mouse_input.middle_mouse_drag_delta_.x,
              mouse_input.middle_mouse_drag_delta_.y);

  ImGui::Separator();

  // Right Mouse Button section
  ImGui::Text("Right Mouse Button");
  ImGui::PushID("RightMouse");
  DisplayBoolValue("Pressed", mouse_input.right_mouse_pressed_);
  DisplayBoolValue("Clicked", mouse_input.right_mouse_clicked_);
  DisplayBoolValue("Released", mouse_input.right_mouse_released_);
  DisplayBoolValue("Double Clicked", mouse_input.right_mouse_double_clicked_);
  DisplayBoolValue("Dragging", mouse_input.right_mouse_dragging_);
  ImGui::PopID();
  ImGui::Text("  Drag Delta: (%.2f, %.2f)", mouse_input.right_mouse_drag_delta_.x,
              mouse_input.right_mouse_drag_delta_.y);

  ImGui::Separator();

  // Mouse Wheel section
  ImGui::Text("Mouse Wheel");
  ImGui::Text("  Vertical Delta: %.2f", mouse_input.mouse_wheel_delta_);
  ImGui::Text("  Horizontal Delta: %.2f", mouse_input.mouse_wheel_h_);

  ImGui::End();
}

}  // namespace vivid::input
