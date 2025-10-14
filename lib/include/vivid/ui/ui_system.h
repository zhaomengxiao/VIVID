#pragma once

#include <flecs.h>

#include "ui_component.h"
#include "vivid/app/App.h"
#include "vivid/render/render_systems.h"
#include "vivid/window/window_component.h"

namespace VIVID {
namespace UI {

struct ShutdownPhase {};  // Custom phase for cleanup systems

// UI Systems Module - manages ImGui lifecycle and rendering
struct UISystems {
  UISystems(flecs::world& world);

private:
  // Static member functions for system implementations
  static void initImGuiImpl(flecs::entity e, WINDOW::WindowGpuComponent& gpu_comp,
                            RENDER::WebGPUResources& webgpuRes);
  static void processImGuiEventImpl(flecs::entity e, VIVID::APP::EventQueues& eventQueues);
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
  world.system<WINDOW::WindowGpuComponent, RENDER::WebGPUResources>("InitImGui")
      .term_at(1)
      .src<RENDER::WebGPUResources>()
      .kind(flecs::OnStart)
      .each(initImGuiImpl);

  // 2. Process ImGui events - runs every frame in PreUpdate
  world
      .system<VIVID::APP::EventQueues>("ProcessImGuiEvent")
      // .term_at(0)
      // .src<VIVID::APP::EventQueues>()
      .kind(flecs::PreUpdate)
      .each(processImGuiEventImpl);

  // 3. Show ImGui demo - runs every frame in Update
  world.system("ShowImGuiDemo").kind(flecs::PreUpdate).run(showImGuiDemoImpl);

  // 4. Shutdown ImGui - runs once at shutdown
  world.system("ShutDownImGui").kind<ShutdownPhase>().run(shutDownImGuiImpl);
}

}  // namespace UI
}  // namespace VIVID
