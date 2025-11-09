#include "inspector_systems.h"

#include <imgui.h>

namespace editor {
namespace inspector {

void InspectorSystems::drawSceneHierarchy(const flecs::iter& it) {
  // World handle for queries and entity creation
  flecs::world world = it.world();

  // auto cubeEntity = world.lookup("MyCube");
  // auto lightEntity = world.lookup("PointLight");
  // auto cameraEntity = world.lookup("MainCamera");

  // Persist selection across frames within this translation unit
  static flecs::entity s_Selected = flecs::entity::null();

  ImGui::Begin("Scene");
  // world.query<>("").each([&](flecs::entity e) { ImGui::Text("Entity: %s", e.name().c_str()); });
  // List all entities that have names
  auto cubeEntity = world.lookup("MyCube");
  auto lightEntity = world.lookup("PointLight");
  auto cameraEntity = world.lookup("MainCamera");
  auto inspectorTest1 = world.lookup("editor::inspector::InspectorSystems::InspectorTest1");
  auto inspectorTest2 = world.lookup("editor::inspector::InspectorSystems::InspectorTest2");

  ImGui::Text("Lookup results:");
  if (cubeEntity.id() != 0) {
    ImGui::Text("  MyCube found (ID: %llu)", (unsigned long long)cubeEntity.id());
  } else {
    ImGui::Text("  MyCube NOT found");
  }
  if (lightEntity.id() != 0) {
    ImGui::Text("  PointLight found (ID: %llu)", (unsigned long long)lightEntity.id());
  } else {
    ImGui::Text("  PointLight NOT found");
  }
  if (cameraEntity.id() != 0) {
    ImGui::Text("  MainCamera found (ID: %llu)", (unsigned long long)cameraEntity.id());
  } else {
    ImGui::Text("  MainCamera NOT found");
  }

  if (inspectorTest1.id() != 0) {
    ImGui::Text("  InspectorTest1 found (ID: %llu)", (unsigned long long)inspectorTest1.id());
  } else {
    ImGui::Text("  InspectorTest1 NOT found");
  }
  if (inspectorTest2.id() != 0) {
    ImGui::Text("  InspectorTest2 found (ID: %llu)", (unsigned long long)inspectorTest2.id());
  } else {
    ImGui::Text("  InspectorTest2 NOT found");
  }

  world.each([&](flecs::entity e) {
    // const char* name = e.name();
    // if (!name) return; // Skip entities without names

    // ImGuiTreeNodeFlags flags = ((s_Selected == e) ? ImGuiTreeNodeFlags_Selected : 0) |
    // ImGuiTreeNodeFlags_OpenOnArrow; flags |= ImGuiTreeNodeFlags_SpanAvailWidth; bool opened =
    // ImGui::TreeNodeEx((void *)(uint64_t)e.id(), flags, "%s", name);

    // if (ImGui::IsItemClicked())
    // {
    //     s_Selected = e;
    // }

    // if (opened)
    // {
    //     // TODO: draw children when hierarchy relationships exist
    //     ImGui::TreePop();
    // }
    ImGui::Text("Entity: %s", e.name().c_str());
  });

  // Context menu on empty space to create a new entity
  if (ImGui::BeginPopupContextWindow()) {
    if (ImGui::MenuItem("Create Empty Entity")) {
      auto entity = world.entity("Empty Entity");
      s_Selected = entity;
    }
    if (ImGui::MenuItem("Create InspectorTest1")) {
      auto entity = world.entity("InspectorTest1");
      s_Selected = entity;
    }
    if (ImGui::MenuItem("Create InspectorTest2")) {
      auto entity = world.entity("InspectorTest2");
      s_Selected = entity;
    }
    ImGui::EndPopup();
  }

  ImGui::Text("drawSceneHierarchy called");
  ImGui::Button("Button");

  ImGui::End();
}

}  // namespace inspector
}  // namespace editor