#pragma once

#include <flecs.h>

#include "ui_component.h"

namespace VIVID {
namespace UI {

struct ShutdownPhase {};  // Custom phase for cleanup systems

// UI Systems Module - manages ImGui lifecycle and rendering
struct UISystems {
  UISystems(flecs::world& world);

private:
  // Static member functions for system implementations
  static void initImGuiImpl(flecs::iter& it);
  static void processImGuiEventImpl(flecs::iter& it);
  static void showImGuiDemoImpl(flecs::iter& it);
  static void shutDownImGuiImpl(flecs::iter& it);
};

// Constructor - Register module and systems
inline UISystems::UISystems(flecs::world& world) {
  // Register module
  world.module<UISystems>();

  // Import components module
  world.import <UIComponents>();

  // Register systems
  // 1. Initialize ImGui - deferred to PreUpdate to see OnStart changes and after WebGPU init
  world.system("InitImGui").kind(flecs::PreUpdate).run(initImGuiImpl);

  // 2. Process ImGui events - runs every frame in PreUpdate
  world.system("ProcessImGuiEvent").kind(flecs::PreUpdate).run(processImGuiEventImpl);

  // 3. Show ImGui demo - runs every frame in Update
  world.system("ShowImGuiDemo").kind(flecs::PreUpdate).run(showImGuiDemoImpl);

  // 4. Shutdown ImGui - runs once at shutdown
  world.system("ShutDownImGui").kind<ShutdownPhase>().run(shutDownImGuiImpl);
}

}  // namespace UI
}  // namespace VIVID
