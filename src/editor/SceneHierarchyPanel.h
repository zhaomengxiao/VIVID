#pragma once

#include <entt/entt.hpp>

class SceneHierarchyPanel
{
public:
    SceneHierarchyPanel() = default;
    SceneHierarchyPanel(const entt::registry *context);

    void SetContext(const entt::registry *context);

    void OnImGuiRender();

private:
    const entt::registry *m_Context = nullptr;
    entt::entity m_SelectionContext;
};