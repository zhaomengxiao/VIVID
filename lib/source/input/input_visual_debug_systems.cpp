#include "vivid/input/input_visual_debug_systems.h"

#include <imgui.h>
#include <vivid/input/input_component.h>
#include <vivid/log/log.h>

#include <cstdint>
#include <initializer_list>
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

  // Register systems to display input debug panels
  world.system<MouseInputResource>("DisplayMouseInputDebug")
      .kind(flecs::OnUpdate)
      .term_at(0)
      .src<MouseInputResource>()
      .each(displayMouseInputDebugImpl);
  world.system<KeyboardInputResource>("DisplayKeyboardInputDebug")
      .kind(flecs::OnUpdate)
      .term_at(0)
      .src<KeyboardInputResource>()
      .each(displayKeyboardInputDebugImpl);

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

// Display keyboard input debug panel - shows all KeyboardInputResource values in keyboard layout
void InputVisualDebugSystems::displayKeyboardInputDebugImpl(
    const KeyboardInputResource& keyboard_input) {
  if (!ImGui::Begin("Keyboard Input Debug")) {
    ImGui::End();
    return;
  }

  // Set up button styling for pressed keys
  const ImVec4 kPressedColor = ImVec4(0.2F, 0.85F, 0.3F, 1.0F);  // Bright green for pressed keys
  const ImVec4 kPressedTextColor
      = ImVec4(0.05F, 0.1F, 0.05F, 1.0F);  // Dark text on green background

  // Base palette inspired by reference keyboard
  const ImVec4 kKeyIdleColor = ImVec4(0.18F, 0.18F, 0.2F, 1.0F);
  const ImVec4 kKeyHoverColor = ImVec4(0.23F, 0.23F, 0.27F, 1.0F);
  const ImVec4 kKeyActiveColor = ImVec4(0.15F, 0.15F, 0.18F, 1.0F);
  const ImVec4 kKeyTextColor = ImVec4(0.84F, 0.86F, 0.9F, 1.0F);
  const ImVec4 kAccentIdleColor = ImVec4(0.15F, 0.35F, 0.85F, 1.0F);
  const ImVec4 kAccentHoverColor = ImVec4(0.18F, 0.4F, 0.95F, 1.0F);
  const ImVec4 kAccentActiveColor = ImVec4(0.12F, 0.28F, 0.7F, 1.0F);
  const ImVec4 kAccentTextColor = ImVec4(1.0F, 1.0F, 1.0F, 1.0F);
  const ImVec4 kModifierIdleColor = ImVec4(0.13F, 0.13F, 0.15F, 1.0F);
  const ImVec4 kModifierHoverColor = ImVec4(0.17F, 0.17F, 0.2F, 1.0F);
  const ImVec4 kModifierActiveColor = ImVec4(0.1F, 0.1F, 0.12F, 1.0F);

  // Key layout constants tuned to resemble a physical keyboard
  constexpr float kKeyHeight = 34.0F;
  constexpr float kUnitWidth = 40.0F;  // Base width for letter keys
  constexpr float kKeySpacing = 6.0F;
  constexpr float kRowSpacing = 4.0F;

  auto width_for_units = [&](float units) {
    const float kWidthExtra = units > 1.0F ? units - 1.0F : 0.0F;
    return (kUnitWidth * units) + (kKeySpacing * kWidthExtra);
  };

  enum class KeyVariant : std::uint8_t { kDefault = 0, kAccent, kModifier };

  struct KeyDescriptor {
    bool is_spacer_;
    ImGuiKey key_;
    const char* label_;
    float width_units_;
    const char* unique_suffix_;
    float spacer_units_;
    KeyVariant variant_;
  };

  auto make_key = [](ImGuiKey key, const char* label, float width_units = 1.0F,
                     const char* suffix = nullptr, KeyVariant variant = KeyVariant::kDefault) {
    return KeyDescriptor{false, key, label, width_units, suffix, 0.0F, variant};
  };
  auto make_spacer = [](float spacer_units) {
    return KeyDescriptor{true,         ImGuiKey_None,       nullptr, 0.0F, nullptr,
                         spacer_units, KeyVariant::kDefault};
  };

  // Helper function to display a key button with unique ID
  auto display_key_button = [&](ImGuiKey key, const char* label, float width, float height,
                                const char* unique_suffix, KeyVariant variant) {
    const bool kIsPressed = keyboard_input.IsKeyPressed(key);
    int variant_colors_pushed = 0;

    auto push_variant_colors = [&](const ImVec4& base, const ImVec4& hover, const ImVec4& active,
                                   const ImVec4* text_color = nullptr) {
      ImGui::PushStyleColor(ImGuiCol_Button, base);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, active);
      variant_colors_pushed += 3;
      if (text_color != nullptr) {
        ImGui::PushStyleColor(ImGuiCol_Text, *text_color);
        ++variant_colors_pushed;
      }
    };

    if (!kIsPressed) {
      switch (variant) {
        case KeyVariant::kAccent:
          push_variant_colors(kAccentIdleColor, kAccentHoverColor, kAccentActiveColor,
                              &kAccentTextColor);
          break;
        case KeyVariant::kModifier:
          push_variant_colors(kModifierIdleColor, kModifierHoverColor, kModifierActiveColor);
          break;
        default:
          break;
      }
    }

    // Create unique label with ID suffix to avoid conflicts
    std::string unique_label = label;
    if (unique_suffix != nullptr) {
      unique_label += "##";
      unique_label += unique_suffix;
    } else {
      // Use key enum value as unique identifier
      unique_label += "##key_";
      unique_label += std::to_string(static_cast<int>(key));
    }

    if (kIsPressed) {
      ImGui::PushStyleColor(ImGuiCol_Button, kPressedColor);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kPressedColor);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, kPressedColor);
      ImGui::PushStyleColor(ImGuiCol_Text, kPressedTextColor);
    }

    ImGui::Button(unique_label.c_str(), ImVec2(width, height));

    if (kIsPressed) {
      ImGui::PopStyleColor(4);
    } else if (variant_colors_pushed > 0) {
      ImGui::PopStyleColor(variant_colors_pushed);
    }
  };

  // Helper that keeps each row compact and evenly spaced
  auto draw_row = [&](std::initializer_list<KeyDescriptor> row, float indent_units = 0.0F) {
    if (indent_units > 0.0F) {
      const float kIndentPx = indent_units * (kUnitWidth + kKeySpacing);
      ImGui::Dummy(ImVec2(kIndentPx, 0));
      ImGui::SameLine(0, 0);
    }

    size_t index = 0;
    for (const auto& key : row) {
      if (key.is_spacer_) {
        const float kSpacerPx = key.spacer_units_ * (kUnitWidth + kKeySpacing);
        ImGui::Dummy(ImVec2(kSpacerPx, kKeyHeight));
      } else {
        const float kKeyWidth = width_for_units(key.width_units_ > 0.0F ? key.width_units_ : 1.0F);
        display_key_button(key.key_, key.label_, kKeyWidth, kKeyHeight, key.unique_suffix_,
                           key.variant_);
      }

      if (++index < row.size()) {
        ImGui::SameLine(0, kKeySpacing);
      }
    }

    ImGui::NewLine();
    ImGui::Dummy(ImVec2(0, kRowSpacing));
  };

  ImGui::TextColored(ImVec4(0.8F, 0.8F, 0.8F, 1.0F), "Keyboard Layout (Green = Pressed)");
  ImGui::Spacing();

  ImGui::PushStyleColor(ImGuiCol_Button, kKeyIdleColor);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kKeyHoverColor);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, kKeyActiveColor);
  ImGui::PushStyleColor(ImGuiCol_Text, kKeyTextColor);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0F);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0F, 8.0F));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(kKeySpacing, kRowSpacing));

  ImGui::BeginGroup();
  draw_row({
      make_key(ImGuiKey_Escape, "Esc"),
      make_spacer(0.75F),
      make_key(ImGuiKey_F1, "F1"),
      make_key(ImGuiKey_F2, "F2"),
      make_key(ImGuiKey_F3, "F3"),
      make_key(ImGuiKey_F4, "F4"),
      make_spacer(0.5F),
      make_key(ImGuiKey_F5, "F5"),
      make_key(ImGuiKey_F6, "F6"),
      make_key(ImGuiKey_F7, "F7"),
      make_key(ImGuiKey_F8, "F8"),
      make_spacer(0.5F),
      make_key(ImGuiKey_F9, "F9"),
      make_key(ImGuiKey_F10, "F10"),
      make_key(ImGuiKey_F11, "F11"),
      make_key(ImGuiKey_F12, "F12"),
  });

  draw_row({
      make_key(ImGuiKey_GraveAccent, "`"),
      make_key(ImGuiKey_1, "1"),
      make_key(ImGuiKey_2, "2"),
      make_key(ImGuiKey_3, "3"),
      make_key(ImGuiKey_4, "4"),
      make_key(ImGuiKey_5, "5"),
      make_key(ImGuiKey_6, "6"),
      make_key(ImGuiKey_7, "7"),
      make_key(ImGuiKey_8, "8"),
      make_key(ImGuiKey_9, "9"),
      make_key(ImGuiKey_0, "0"),
      make_key(ImGuiKey_Minus, "-"),
      make_key(ImGuiKey_Equal, "="),
      make_key(ImGuiKey_Backspace, "Backspace", 2.0F),
  });

  draw_row({
      make_key(ImGuiKey_Tab, "Tab", 1.5F),
      make_key(ImGuiKey_Q, "Q"),
      make_key(ImGuiKey_W, "W"),
      make_key(ImGuiKey_E, "E"),
      make_key(ImGuiKey_R, "R"),
      make_key(ImGuiKey_T, "T"),
      make_key(ImGuiKey_Y, "Y"),
      make_key(ImGuiKey_U, "U"),
      make_key(ImGuiKey_I, "I"),
      make_key(ImGuiKey_O, "O"),
      make_key(ImGuiKey_P, "P"),
      make_key(ImGuiKey_LeftBracket, "["),
      make_key(ImGuiKey_RightBracket, "]"),
      make_key(ImGuiKey_Backslash, "\\", 1.5F),
  });

  draw_row({
      make_key(ImGuiKey_CapsLock, "Caps", 1.75F, nullptr, KeyVariant::kModifier),
      make_key(ImGuiKey_A, "A"),
      make_key(ImGuiKey_S, "S"),
      make_key(ImGuiKey_D, "D"),
      make_key(ImGuiKey_F, "F"),
      make_key(ImGuiKey_G, "G"),
      make_key(ImGuiKey_H, "H"),
      make_key(ImGuiKey_J, "J"),
      make_key(ImGuiKey_K, "K"),
      make_key(ImGuiKey_L, "L"),
      make_key(ImGuiKey_Semicolon, ";"),
      make_key(ImGuiKey_Apostrophe, "'"),
      make_key(ImGuiKey_Enter, "Enter", 2.25F, nullptr, KeyVariant::kAccent),
  });

  draw_row({
      make_key(ImGuiKey_LeftShift, "Shift", 2.25F, "Left", KeyVariant::kModifier),
      make_key(ImGuiKey_Z, "Z"),
      make_key(ImGuiKey_X, "X"),
      make_key(ImGuiKey_C, "C"),
      make_key(ImGuiKey_V, "V"),
      make_key(ImGuiKey_B, "B"),
      make_key(ImGuiKey_N, "N"),
      make_key(ImGuiKey_M, "M"),
      make_key(ImGuiKey_Comma, ","),
      make_key(ImGuiKey_Period, "."),
      make_key(ImGuiKey_Slash, "/"),
      make_key(ImGuiKey_RightShift, "Shift", 2.75F, "Right", KeyVariant::kModifier),
  });

  draw_row({
      make_key(ImGuiKey_LeftCtrl, "Ctrl", 1.25F, "Left", KeyVariant::kModifier),
      make_key(ImGuiKey_LeftSuper, "Win", 1.25F, "Left", KeyVariant::kModifier),
      make_key(ImGuiKey_LeftAlt, "Alt", 1.25F, "Left", KeyVariant::kModifier),
      make_key(ImGuiKey_Space, "Space", 6.25F),
      make_key(ImGuiKey_RightAlt, "Alt", 1.25F, "Right", KeyVariant::kModifier),
      make_key(ImGuiKey_Menu, "Menu", 1.25F, nullptr, KeyVariant::kModifier),
      make_key(ImGuiKey_RightCtrl, "Ctrl", 1.25F, "Right", KeyVariant::kModifier),
  });
  ImGui::EndGroup();

  ImGui::SameLine(0, 24.0F);

  // Navigation + arrow cluster
  ImGui::BeginGroup();
  draw_row({
      make_key(ImGuiKey_Insert, "Insert", 1.0F, nullptr, KeyVariant::kModifier),
      make_key(ImGuiKey_Home, "Home", 1.0F, nullptr, KeyVariant::kModifier),
      make_key(ImGuiKey_PageUp, "PgUp", 1.0F, nullptr, KeyVariant::kModifier),
  });

  draw_row({
      make_key(ImGuiKey_Delete, "Delete", 1.0F, nullptr, KeyVariant::kModifier),
      make_key(ImGuiKey_End, "End", 1.0F, nullptr, KeyVariant::kModifier),
      make_key(ImGuiKey_PageDown, "PgDn", 1.0F, nullptr, KeyVariant::kModifier),
  });

  draw_row({
      make_spacer(0.5F),
      make_key(ImGuiKey_UpArrow, "^"),
      make_spacer(0.5F),
  });

  draw_row({
      make_key(ImGuiKey_LeftArrow, "<", 1.0F, nullptr, KeyVariant::kModifier),
      make_key(ImGuiKey_DownArrow, "v", 1.0F, nullptr, KeyVariant::kModifier),
      make_key(ImGuiKey_RightArrow, ">", 1.0F, nullptr, KeyVariant::kModifier),
  });
  ImGui::EndGroup();

  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(4);

  ImGui::End();
}

}  // namespace vivid::input
