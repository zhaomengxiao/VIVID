#include "SceneHierarchyPanel.h"
#include "components/rendering_components.h" // For TagComponent

#include <imgui.h>

// Forward declaration
static void DrawEntityNode(entt::entity entity, const entt::registry *registry, entt::entity &selectionContext);

SceneHierarchyPanel::SceneHierarchyPanel(const entt::registry *context)
{
    SetContext(context);
}

void SceneHierarchyPanel::SetContext(const entt::registry *context)
{
    m_Context = context;
}

void SceneHierarchyPanel::OnImGuiRender()
{
    ImGui::Begin("Scene");

    if (m_Context)
    {
        for (auto entityID : m_Context->view<TagComponent>())
            DrawEntityNode(entityID, m_Context, m_SelectionContext);
    }

    ImGui::End();
}

static void DrawEntityNode(entt::entity entity, const entt::registry *registry, entt::entity &selectionContext)
{
    auto &tag = registry->get<TagComponent>(entity).Tag;

    ImGuiTreeNodeFlags flags = ((selectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
    bool opened = ImGui::TreeNodeEx((void *)(uint64_t)(uint32_t)entity, flags, tag.c_str());

    if (ImGui::IsItemClicked())
    {
        selectionContext = entity;
    }

    if (opened)
    {
        // TODO: draw children
        ImGui::TreePop();
    }
}