#include "inspector_systems.h"

#include <imgui.h>

namespace editor {
namespace inspector {


void InspectorSystems::drawSceneHierarchy(const flecs::iter& it)
{

    // World handle for queries and entity creation
    flecs::world world = it.world();

    // Persist selection across frames within this translation unit
    static flecs::entity s_Selected = flecs::entity::null();

    ImGui::Begin("Scene");

    // List all entities that have names
    world.each([&](flecs::entity e) {
        // const char* name = e.name();
        // if (!name) return; // Skip entities without names

        // ImGuiTreeNodeFlags flags = ((s_Selected == e) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        // flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
        // bool opened = ImGui::TreeNodeEx((void *)(uint64_t)e.id(), flags, "%s", name);

        // if (ImGui::IsItemClicked())
        // {
        //     s_Selected = e;
        // }

        // if (opened)
        // {
        //     // TODO: draw children when hierarchy relationships exist
        //     ImGui::TreePop();
        // }
        vivid::
    });

    // Context menu on empty space to create a new entity
    if (ImGui::BeginPopupContextWindow())
    {
        if (ImGui::MenuItem("Create Empty Entity"))
        {
            auto entity = world.entity("Empty Entity");
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