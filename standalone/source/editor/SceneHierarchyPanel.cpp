#include "SceneHierarchyPanel.h"
#include <vivid/rendering/render_component.h> // For TagComponent

#include <imgui.h>

// Forward declaration
static void DrawEntityNode(entt::entity entity, entt::registry *registry, entt::entity &selectionContext);

SceneHierarchyPanel::SceneHierarchyPanel(entt::registry *context)
{
    SetContext(context);
}

void SceneHierarchyPanel::SetContext(entt::registry *context)
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

        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::MenuItem("Create Empty Entity"))
            {
                CreateEntity("Empty Entity");
            }

            ImGui::EndPopup();
        }
    }

    ImGui::End();
}

void SceneHierarchyPanel::CreateEntity(const std::string &name)
{
    entt::entity entity = m_Context->create();
    m_Context->emplace<TagComponent>(entity, name);
    m_SelectionContext = entity;
}

static void DrawEntityNode(entt::entity entity, entt::registry *registry, entt::entity &selectionContext)
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