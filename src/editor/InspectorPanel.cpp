#include "InspectorPanel.h"
#include "components/rendering_components.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <entt/core/hashed_string.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>

namespace
{
    // Constants
    constexpr size_t INPUT_BUFFER_SIZE = 256;
    constexpr float DEFAULT_DRAG_SPEED = 0.1f;
    constexpr float DEFAULT_COLUMN_WIDTH = 100.0f;

    using namespace entt::literals;

    // Get the default reset value for a field
    float GetResetValue(entt::id_type data_id)
    {
        return (data_id == "Scale"_hs) ? 1.0f : 0.0f;
    }

    // Improved Vec3 control with error handling
    bool DrawVec3Control(const std::string &label, glm::vec3 &values, float resetValue = 0.0f, float columnWidth = DEFAULT_COLUMN_WIDTH)
    {
        if (label.empty())
            return false;

        bool modified = false;

        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text("%s", label.c_str()); // Prevent format string exploits
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

        // X-axis control
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
        if (ImGui::Button("X", buttonSize))
        {
            values.x = resetValue;
            modified = true;
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::DragFloat("##X", &values.x, DEFAULT_DRAG_SPEED, 0.0f, 0.0f, "%.2f"))
            modified = true;
        ImGui::PopItemWidth();
        ImGui::SameLine();

        // Y-axis control
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
        if (ImGui::Button("Y", buttonSize))
        {
            values.y = resetValue;
            modified = true;
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::DragFloat("##Y", &values.y, DEFAULT_DRAG_SPEED, 0.0f, 0.0f, "%.2f"))
            modified = true;
        ImGui::PopItemWidth();
        ImGui::SameLine();

        // Z-axis control
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
        if (ImGui::Button("Z", buttonSize))
        {
            values.z = resetValue;
            modified = true;
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::DragFloat("##Z", &values.z, DEFAULT_DRAG_SPEED, 0.0f, 0.0f, "%.2f"))
            modified = true;
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();
        ImGui::Columns(1);
        ImGui::PopID();

        return modified;
    }

    // Core function: dynamically render data members with error handling
    bool DrawDataMember(const entt::meta_data &metaData, entt::meta_any &componentInstance)
    {
        const char *memberName = metaData.name();
        if (!memberName)
            return false;

        auto data_id = metaData.type().id();
        auto getter = metaData.get(componentInstance);
        if (!getter)
            return false;

        bool modified = false;
        auto memberType = metaData.type();

        // Type-driven UI generation
        if (memberType == entt::resolve<glm::vec3>())
        {
            auto vec3 = getter.cast<glm::vec3>();
            float resetVal = GetResetValue(data_id);
            if (DrawVec3Control(memberName, vec3, resetVal))
            {
                if (metaData.set(componentInstance, vec3))
                {
                    modified = true;
                }
            }
        }
        else if (memberType == entt::resolve<bool>())
        {
            auto value = getter.cast<bool>();
            if (ImGui::Checkbox(memberName, &value))
            {
                if (metaData.set(componentInstance, value))
                {
                    modified = true;
                }
            }
        }
        else if (memberType == entt::resolve<std::string>())
        {
            auto value = getter.cast<std::string>();
            static char buffer[INPUT_BUFFER_SIZE]; // Use a static buffer to avoid frequent allocations

            // Safe string copy
            size_t copyLen = std::min(value.length(), INPUT_BUFFER_SIZE - 1);
            std::memcpy(buffer, value.c_str(), copyLen);
            buffer[copyLen] = '\0';

            ImGui::PushID(memberName);
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, DEFAULT_COLUMN_WIDTH);
            ImGui::Text("%s", memberName);
            ImGui::NextColumn();

            if (ImGui::InputText("##Value", buffer, INPUT_BUFFER_SIZE))
            {
                if (metaData.set(componentInstance, std::string(buffer)))
                {
                    modified = true;
                }
            }

            ImGui::Columns(1);
            ImGui::PopID();
        }
        else if (memberType == entt::resolve<float>())
        {
            auto value = getter.cast<float>();
            if (ImGui::DragFloat(memberName, &value, DEFAULT_DRAG_SPEED))
            {
                if (metaData.set(componentInstance, value))
                {
                    modified = true;
                }
            }
        }
        else if (memberType == entt::resolve<int>())
        {
            auto value = getter.cast<int>();
            if (ImGui::DragInt(memberName, &value))
            {
                if (metaData.set(componentInstance, value))
                {
                    modified = true;
                }
            }
        }
        else
        {
            // For unsupported types, display type information
            ImGui::Text("%s: %s (Editing not supported)", memberName,
                        memberType.name() ? memberType.name() : "Unknown Type");
        }

        return modified;
    }

    // Draw a single component
    void DrawComponent(entt::meta_type &metaType, entt::meta_any &componentInstance)
    {
        const char *componentName = metaType.name();
        if (!componentName)
        {
            componentName = "Unknown Component";
        }

        if (ImGui::CollapsingHeader(componentName, ImGuiTreeNodeFlags_DefaultOpen))
        {
            // Iterate over all data members
            for (auto &&[data_id, meta_data] : metaType.data())
            {
                DrawDataMember(meta_data, componentInstance);
            }
        }
    }
} // namespace

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

    // Check entity validity
    if (selectedEntity == entt::null || !m_Context || !m_Context->valid(selectedEntity))
    {
        ImGui::Text("Please select a valid entity");
        ImGui::End();
        return;
    }

    // Iterate through all storages to find components owned by the entity
    bool hasComponents = false;
    for (auto &&[id, storage] : m_Context->storage())
    {
        if (!storage.contains(selectedEntity))
            continue;

        hasComponents = true;
        auto metaType = entt::resolve(storage.info());
        if (!metaType)
            continue;

        // Get the raw component pointer and wrap it as a meta_any
        void *componentPtr = storage.value(selectedEntity);
        if (!componentPtr)
            continue;

        entt::meta_any componentInstance = metaType.from_void(componentPtr);
        if (!componentInstance)
            continue;

        DrawComponent(metaType, componentInstance);
    }

    if (!hasComponents)
    {
        ImGui::Text("This entity has no components");
    }

    ImGui::End();
}