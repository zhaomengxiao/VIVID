#include "editor_plugin.h"

#include <vivid/app/App.h>

#include "ComponentRegistry.h"

// System implementations for UI
namespace {

  void ui_startup_system(Resources &res, entt::registry &world) {
    VIVID::ComponentRegistry::RegisterAllComponents();
    // --- ImGui Setup ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    float scale = 2.0f;
    float fontSize = 16.0f;
    io.Fonts->AddFontFromFileTTF(
        "D:/ClineWorkSpace/VIVID/build/release/standalone/Release/res/fonts/NotoSans-Regular.ttf",
        fontSize * scale);
    ImGui::StyleColorsDark();
    ImGui::GetStyle().ScaleAllSizes(scale);

    auto *windowRes = res.get<WindowResource>();
    if (windowRes) {
      ImGui_ImplGlfw_InitForOpenGL(windowRes->window, true);
    } else {
      std::cerr << "Error: WindowResource not found for ImGui initialization." << std::endl;
    }
    ImGui_ImplOpenGL3_Init("#version 330");

    // --- Create & Register UI Panels as Resources ---
    res.insert<SceneHierarchyPanel>().SetContext(&world);
    res.insert<InspectorPanel>().SetContext(&world);
  }

  void ui_update_system(Resources &res, entt::registry &world) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport();

    auto *sceneHierarchyPanel = res.get<SceneHierarchyPanel>();
    auto *inspectorPanel = res.get<InspectorPanel>();

    if (sceneHierarchyPanel) {
      sceneHierarchyPanel->OnImGuiRender();
      if (inspectorPanel) {
        auto selectedEntity = sceneHierarchyPanel->GetSelectedEntity();
        inspectorPanel->OnImGuiRender(selectedEntity);
      }
    }

    ImGui::Begin("Viewport");
    world.view<ViewportComponent, CameraComponent>().each(
        [](auto entity, ViewportComponent &viewport, CameraComponent & /* camera */) {
          ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
          viewport.Width = viewportPanelSize.x;
          viewport.Height = viewportPanelSize.y;

          uint32_t textureID = viewport.TextureID;
          ImGui::Image((ImTextureID)(intptr_t)textureID, ImVec2(viewport.Width, viewport.Height),
                       ImVec2(0, 1), ImVec2(1, 0));
        });
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }

  void ui_shutdown_system(Resources &, entt::registry &) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }

}  // anonymous namespace

void EditorPlugin::build(App &app) {
  app.add_system(ScheduleLabel::Startup, ui_startup_system);
  app.add_system(ScheduleLabel::Update, ui_update_system);
  app.add_system(ScheduleLabel::Shutdown, ui_shutdown_system);
}

std::string EditorPlugin::name() const { return "EditorPlugin"; }