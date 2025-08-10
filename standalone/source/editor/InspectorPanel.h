#pragma once

#include <entt/entt.hpp>

class InspectorPanel
{
public:
    InspectorPanel() = default;
    InspectorPanel(entt::registry *context);

    void SetContext(entt::registry *context);

    void OnImGuiRender(entt::entity selectedEntity);

private:
    void DrawAddComponentButton(entt::entity selectedEntity);

    entt::registry *m_Context = nullptr;
};