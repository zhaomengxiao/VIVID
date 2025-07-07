#pragma once

#include <entt/entt.hpp>

class SceneHierarchyPanel
{
public:
    SceneHierarchyPanel() = default;
    SceneHierarchyPanel(entt::registry *context);

    void SetContext(entt::registry *context);

    void OnImGuiRender();

    entt::entity GetSelectedEntity() const { return m_SelectionContext; }

private:
    void CreateEntity(const std::string &name);

    entt::registry *m_Context = nullptr;
    entt::entity m_SelectionContext{entt::null};
};