#include <vivid/input/camera_controller.h>
#include <vivid/rendering/render_plugin.h>

#include <entt/entity/registry.hpp>
#include <iostream>

#include "../../standalone/source/editor/ComponentRegistry.h"
#include "vivid/app/App.h"
#include "vivid/rendering/render_component.h"
#include "vivid/rendering/render_system.h"

namespace {
  // Helper function to keep renderer in sync with component changes
  void OnComponentChange(entt::registry &registry, entt::entity entity) {
    // This function is triggered whenever a MeshComponent or MaterialComponent
    // is added or updated. It calls the RendererSystem's Sync function
    // to ensure GPU resources are created or updated accordingly.
    VIVID::RendererSystem::Sync(registry);
    std::cout << "Component changed" << std::endl;
  }

  void render_startup_system(Resources &, entt::registry &world) {
    // --- Register Components ---
    // VIVID::ComponentRegistry::RegisterAllComponents();

    // --- ECS Listeners ---
    world.on_construct<MeshComponent>().connect<&OnComponentChange>();
    world.on_update<MeshComponent>().connect<&OnComponentChange>();
    world.on_construct<MaterialComponent>().connect<&OnComponentChange>();
    world.on_update<MaterialComponent>().connect<&OnComponentChange>();

    // --- Renderer Init ---
    VIVID::RendererSystem::Init();
    VIVID::RendererSystem::Sync(world);
  }

  void render_update_system(Resources &, entt::registry &world) {
    VIVID::RendererSystem::Update(world);
  }

  void render_shutdown_system(Resources &, entt::registry &) { VIVID::RendererSystem::Shutdown(); }

}  // anonymous namespace

void RenderPlugin::build(App &app) {
  app.add_system(ScheduleLabel::Startup, render_startup_system);
  app.add_system(ScheduleLabel::Update, render_update_system);
  app.add_system(ScheduleLabel::Shutdown, render_shutdown_system);
}

std::string RenderPlugin::name() const { return "RenderPlugin"; }