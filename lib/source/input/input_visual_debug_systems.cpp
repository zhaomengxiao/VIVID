#include "vivid/input/input_visual_debug_systems.h"

#include <imgui.h>
#include <vivid/input/input_component.h>
#include <vivid/log/log.h>

#include <string>

namespace VIVID::INPUT {

// Color constants for debug display
namespace {
constexpr ImVec4 GREEN_BACKGROUND(0.0F, 0.5F, 0.0F, 1.0F);  // Dark green background for true values

// Helper function to display bool value with green background when true
void DisplayBoolValue(const char* label, bool value) {
  std::string text = "  " + std::string(label) + ": " + (value ? "Yes" : "No");

  // Disable button interaction to make it display-only
  ImGui::BeginDisabled();

  if (value) {
    // Push green background color for the button frame when value is true
    ImGui::PushStyleColor(ImGuiCol_Button, GREEN_BACKGROUND);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GREEN_BACKGROUND);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, GREEN_BACKGROUND);
  }

  // Use Button with full width (-1) to show text with background color that fills the width
  ImGui::Button(text.c_str(), ImVec2(-1, 0));

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
void InputVisualDebugSystems::displayMouseInputDebugImpl(MouseInputResource& mouseInput) {
  if (!ImGui::Begin("Mouse Input Debug")) {
    ImGui::End();
    return;
  }

  // Mouse Position & Delta section
  ImGui::Text("Mouse Position & Delta");
  ImGui::Text("  Position: (%.2f, %.2f)", mouseInput.MousePos.x, mouseInput.MousePos.y);
  ImGui::Text("  Delta: (%.2f, %.2f)", mouseInput.MouseDelta.x, mouseInput.MouseDelta.y);

  ImGui::Separator();

  // Left Mouse Button section
  ImGui::Text("Left Mouse Button");
  ImGui::PushID("LeftMouse");
  DisplayBoolValue("Pressed", mouseInput.MousePressed);
  DisplayBoolValue("Clicked", mouseInput.MouseClicked);
  DisplayBoolValue("Released", mouseInput.MouseReleased);
  DisplayBoolValue("Double Clicked", mouseInput.MouseDoubleClicked);
  DisplayBoolValue("Dragging", mouseInput.MouseDragging);
  ImGui::PopID();
  ImGui::Text("  Drag Delta: (%.2f, %.2f)", mouseInput.LeftMouseDragDelta.x,
              mouseInput.LeftMouseDragDelta.y);

  ImGui::Separator();

  // Middle Mouse Button section
  ImGui::Text("Middle Mouse Button");
  ImGui::PushID("MiddleMouse");
  DisplayBoolValue("Pressed", mouseInput.MiddleMousePressed);
  DisplayBoolValue("Clicked", mouseInput.MiddleMouseClicked);
  DisplayBoolValue("Released", mouseInput.MiddleMouseReleased);
  DisplayBoolValue("Dragging", mouseInput.MiddleMouseDragging);
  ImGui::PopID();
  ImGui::Text("  Drag Delta: (%.2f, %.2f)", mouseInput.MiddleMouseDragDelta.x,
              mouseInput.MiddleMouseDragDelta.y);

  ImGui::Separator();

  // Right Mouse Button section
  ImGui::Text("Right Mouse Button");
  ImGui::PushID("RightMouse");
  DisplayBoolValue("Pressed", mouseInput.RightMousePressed);
  DisplayBoolValue("Clicked", mouseInput.RightMouseClicked);
  DisplayBoolValue("Released", mouseInput.RightMouseReleased);
  DisplayBoolValue("Double Clicked", mouseInput.RightMouseDoubleClicked);
  DisplayBoolValue("Dragging", mouseInput.RightMouseDragging);
  ImGui::PopID();
  ImGui::Text("  Drag Delta: (%.2f, %.2f)", mouseInput.RightMouseDragDelta.x,
              mouseInput.RightMouseDragDelta.y);

  ImGui::Separator();

  // Mouse Wheel section
  ImGui::Text("Mouse Wheel");
  ImGui::Text("  Vertical Delta: %.2f", mouseInput.MouseWheelDelta);
  ImGui::Text("  Horizontal Delta: %.2f", mouseInput.MouseWheelH);

  ImGui::End();
}

}  // namespace VIVID::INPUT
