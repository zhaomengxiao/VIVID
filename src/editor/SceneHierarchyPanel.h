#pragma once

#include <entt/entt.hpp>

class SceneHierarchyPanel
{
public:
    SceneHierarchyPanel() = default;
    SceneHierarchyPanel(const entt::registry *context);

    void SetContext(const entt::registry *context);

    void OnImGuiRender();

    entt::entity GetSelectedEntity() const { return m_SelectionContext; }

private:
    const entt::registry *m_Context = nullptr;
    entt::entity m_SelectionContext{entt::null};
};