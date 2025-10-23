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
  static void renderImGuiImpl(flecs::entity e, RENDER::WebGPUResources& webgpuRes);
  static void shutDownImGuiImpl(flecs::iter& it);
};

// Constructor - Register module and systems
inline UISystems::UISystems(flecs::world& world) {
  VividLogger::app_info("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
  VividLogger::app_info("=============Registering UISystems...~~~~~");
  // Register module
  world.module<UISystems>();

  // Import components module
  world.import <UIComponents>();

  // Import render module //TODO: Test duplicate import
  // world.import <RENDER::RenderComponents>();
  // world.import <RENDER::RenderSystems>();

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

      .kind(flecs::PreUpdate)
      .each(processImGuiEventImpl);

  // 3. Show ImGui demo - runs every frame in Update
  world.system("ShowImGuiDemo").kind(flecs::PreUpdate).run(showImGuiDemoImpl);

  flecs::entity RenderUIPhase = world.lookup("VIVID::RENDER::RenderSystems::RenderUIPhase");
  if (RenderUIPhase.id() == 0) {
    VividLogger::app_error("!!!!!!!!!!!!!!!RenderUIPhase not found, skipping ImGui render...");
    return;
  } else {
    VividLogger::app_debug("!!!!!!!!!!!!!!RenderUIPhase found: %s", RenderUIPhase.name());
  }
  // 3.5 Render ImGui draw data within the render pass
  world.system<RENDER::WebGPUResources>("RenderImGui").kind(RenderUIPhase).each(renderImGuiImpl);

  // 4. Shutdown ImGui - runs once at shutdown
  world.system("ShutDownImGui").kind<ShutdownPhase>().run(shutDownImGuiImpl);
}

}  // namespace UI
}  // namespace VIVID
