#include "InspectorPanel.h"
#include "components/rendering_components.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <entt/core/hashed_string.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>

// A slightly modified helper that returns true if a value was changed.
static bool DrawVec3Control(const std::string &label, glm::vec3 &values, float resetValue = 0.0f, float columnWidth = 100.0f)
{
    bool modified = false;

    ImGui::PushID(label.c_str());

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text(label.c_str());
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

    float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
    if (ImGui::Button("X", buttonSize))
    {
        values.x = resetValue;
        modified = true;
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    if (ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f"))
        modified = true;
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
    if (ImGui::Button("Y", buttonSize))
    {
        values.y = resetValue;
        modified = true;
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    if (ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f"))
        modified = true;
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
    if (ImGui::Button("Z", buttonSize))
    {
        values.z = resetValue;
        modified = true;
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    if (ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f"))
        modified = true;
    ImGui::PopItemWidth();

    ImGui::PopStyleVar();
    ImGui::Columns(1);
    ImGui::PopID();

    return modified;
}

template <typename T>
static void DrawComponentUI(const char *name, entt::registry &registry, entt::entity entity)
{
    using namespace entt::literals;
    if (!registry.all_of<T>(entity))
        return;

    if (ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto &component = registry.get<T>(entity);
        auto metaType = entt::resolve<T>();

        for (auto data_pair : metaType.data())
        {
            auto data_id = data_pair.first;
            auto meta_data = data_pair.second;
            auto getter = meta_data.get(component);

            if (data_id == "Tag"_hs)
            {
                auto value = getter.cast<std::string>();
                char buffer[256];
                strcpy_s(buffer, sizeof(buffer), value.c_str());
                if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
                {
                    meta_data.set(component, std::string(buffer));
                }
            }
            else if (data_id == "Position"_hs)
            {
                auto vec3 = getter.cast<glm::vec3>();
                if (DrawVec3Control("Position", vec3))
                {
                    meta_data.set(component, vec3);
                }
            }
            else if (data_id == "Rotation"_hs)
            {
                auto vec3 = getter.cast<glm::vec3>();
                if (DrawVec3Control("Rotation", vec3))
                {
                    meta_data.set(component, vec3);
                }
            }
            else if (data_id == "Scale"_hs)
            {
                auto vec3 = getter.cast<glm::vec3>();
                if (DrawVec3Control("Scale", vec3, 1.0f))
                {
                    meta_data.set(component, vec3);
                }
            }
            else if (data_id == "IsPrimary"_hs)
            {
                auto value = getter.cast<bool>();
                if (ImGui::Checkbox("Is Primary", &value))
                {
                    meta_data.set(component, value);
                }
            }
        }
    }
}

InspectorPanel::InspectorPanel(entt::registry *context)
{
    SetContext(context);
}

void InspectorPanel::SetContext(entt::registry *context)
{
    m_Context = context;
}

void InspectorPanel::OnImGuiRender(entt::entity selectedEntity)
{
    ImGui::Begin("Inspector");
    if (selectedEntity != entt::null && m_Context->valid(selectedEntity))
    {
        DrawComponentUI<TagComponent>("Tag", *m_Context, selectedEntity);
        DrawComponentUI<TransformComponent>("Transform", *m_Context, selectedEntity);
        DrawComponentUI<CameraComponent>("Camera", *m_Context, selectedEntity);
        // To add more components, just add another DrawComponentUI<MyComponent>(...) line
    }
    ImGui::End();
}