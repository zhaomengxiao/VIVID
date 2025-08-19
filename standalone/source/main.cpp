#include "editor/editor_plugin.h"
#include "vivid/app/App.h"
#include "vivid/input/camera_controller.h"
#include "vivid/plugins/DefaultPlugin.h"
#include "vivid/rendering/render_plugin.h"

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

void app_startup_system(Resources &, entt::registry &world) {
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
}

int main() {
  auto &app = App::new_app()
                  .add_plugin<DefaultPlugin>()
                  .add_plugin<RenderPlugin>()
                  .add_plugin<EditorPlugin>()
                  .add_system(ScheduleLabel::Startup, app_startup_system);
  app.run();

  return 0;
}
