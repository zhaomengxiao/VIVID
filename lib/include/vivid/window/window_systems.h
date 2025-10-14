#pragma once

#include <flecs.h>
#include <vivid/log/log.h>

#include "window_component.h"

namespace VIVID {
namespace WINDOW {

struct ShutdownPhase {};  // Custom phase for cleanup systems

// Window Systems Module - manages window lifecycle, events, and updates
struct WindowSystems {
  // Constructor - Register module and systems
  WindowSystems(flecs::world& world) {
    // Register module
    world.module<WindowSystems>();

    // Import components module
    world.import <WindowComponents>();

    // Create a default window entity if none exists
    auto query = world.query<WindowComponent>();
    bool has_window = false;
    query.each([&](flecs::entity e, WindowComponent& wc) { has_window = true; });

    if (!has_window) {
      // Create default window entity
      auto mainWindow
          = world.entity("MainWindow").set<WindowComponent>({}).set<WindowGpuComponent>({});
      // add GPU component must be here,not in windowInitializationImpl, see Defer mechanism
      VividLogger::app_info("Created default window entity");
    }

    // Register systems
    // 1. Window initialization - runs once at startup
    world.system<WindowComponent, WindowGpuComponent>("WindowInitialization")
        .kind(flecs::OnStart)
        .each(windowInitializationImpl);

    // 2. Window event processing - runs every frame in PreUpdate
    // world.system("WindowEventProcessing").kind(flecs::PreUpdate).run(windowEventProcessingImpl);

    // 3. Window update - runs every frame in Update
    world.system<WindowComponent, WindowGpuComponent>("WindowUpdate")
        .kind(flecs::OnUpdate)
        .each(windowUpdateImpl);

    // 4. Window cleanup - runs once at shutdown
    world.system("WindowCleanup").kind<ShutdownPhase>().run(windowCleanupImpl);
  }

private:
  // Static member functions for system implementations
  static void windowInitializationImpl(flecs::entity e, WindowComponent& window_comp,
                                       WindowGpuComponent& gpu_comp);
  static void windowEventProcessingImpl(flecs::iter& it);
  static void windowUpdateImpl(flecs::entity e, WindowComponent& window_comp,
                               WindowGpuComponent& gpu_comp);
  static void windowCleanupImpl(flecs::iter& it);
};

}  // namespace WINDOW
}  // namespace VIVID
