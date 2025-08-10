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

  // Helper function to create a cube mesh component
  MeshComponent CreateCubeMesh() {
    std::vector<float> vertices
        = {// positions          // normals
           -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.5f,  -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f,
           0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, -0.5f, 0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,

           -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.5f,  -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,
           0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  -0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

           -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  -0.5f, 0.5f,  -0.5f, -1.0f, 0.0f,  0.0f,
           -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  -0.5f, -0.5f, 0.5f,  -1.0f, 0.0f,  0.0f,

           0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.5f,  0.5f,  -0.5f, 1.0f,  0.0f,  0.0f,
           0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,  0.5f,  -0.5f, 0.5f,  1.0f,  0.0f,  0.0f,

           -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.5f,  -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,
           0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  -0.5f, -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,

           -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,
           0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,  0.5f,  0.0f,  1.0f,  0.0f};
    std::vector<unsigned int> indices
        = {0,  1,  2,  2,  3,  0,  4,  5,  6,  6,  7,  4,  8,  9,  10, 10, 11, 8,
           12, 13, 14, 14, 15, 12, 16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20};

    return {vertices, indices, indices.size()};
  }

  void render_startup_system(Resources &, entt::registry &world) {
    // --- Register Components ---
    VIVID::ComponentRegistry::RegisterAllComponents();

    // --- ECS Listeners ---
    world.on_construct<MeshComponent>().connect<&OnComponentChange>();
    world.on_update<MeshComponent>().connect<&OnComponentChange>();
    world.on_construct<MaterialComponent>().connect<&OnComponentChange>();
    world.on_update<MaterialComponent>().connect<&OnComponentChange>();

    // --- Create Entities ---
    auto cubeEntity = world.create();
    world.emplace<TagComponent>(cubeEntity, "MyCube");
    world.emplace<TransformComponent>(cubeEntity);
    world.emplace<MeshComponent>(cubeEntity, CreateCubeMesh());
    world.emplace<MaterialComponent>(
        cubeEntity, MaterialComponent{"D:/ClineWorkSpace/VIVID/build/release/standalone/Release/"
                                      "res/shaders/BlinnPhong.shader",
                                      {1.0f, 0.5f, 0.2f}});

    auto lightEntity = world.create();
    world.emplace<TagComponent>(lightEntity, "PointLight");
    auto &lightTransform = world.emplace<TransformComponent>(lightEntity);
    lightTransform.Position = {1.2f, 1.0f, 2.0f};
    world.emplace<LightComponent>(lightEntity);

    auto cameraEntity = world.create();
    world.emplace<TagComponent>(cameraEntity, "MainCamera");
    auto &camTransform = world.emplace<TransformComponent>(cameraEntity);
    camTransform.Position = {0.0f, 0.0f, 5.0f};
    world.emplace<CameraComponent>(cameraEntity);
    world.emplace<ViewportComponent>(cameraEntity);
    world.emplace<CameraControllerComponent>(cameraEntity);

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