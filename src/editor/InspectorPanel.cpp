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

// **核心函数**: 动态渲染数据成员
static bool DrawDataMember(const entt::meta_data &metaData, entt::meta_any &componentInstance)
{
    using namespace entt::literals;

    // 获取成员名称 - 通过data_id哈希匹配友好名称
    // const char* memberName = "Unknown Member";
    // auto data_id = metaData.id();

    // if (data_id == "Tag"_hs) memberName = "Tag";
    // else if (data_id == "Position"_hs) memberName = "Position";
    // else if (data_id == "Rotation"_hs) memberName = "Rotation";
    // else if (data_id == "Scale"_hs) memberName = "Scale";
    // else if (data_id == "IsPrimary"_hs) memberName = "Is Primary";
    // // 可以继续添加更多映射...
    // else {
    // 后备方案：使用类型名称
    const char *memberName = metaData.name();
    // }
    auto data_id = metaData.type().id();

    auto getter = metaData.get(componentInstance);
    if (!getter)
        return false;

    bool modified = false;
    auto memberType = metaData.type();

    // **类型驱动的UI生成**
    if (memberType == entt::resolve<glm::vec3>())
    {
        auto vec3 = getter.cast<glm::vec3>();
        float resetVal = (data_id == "Scale"_hs) ? 1.0f : 0.0f;
        if (DrawVec3Control(memberName, vec3, resetVal))
        {
            metaData.set(componentInstance, vec3);
            modified = true;
        }
    }
    else if (memberType == entt::resolve<bool>())
    {
        auto value = getter.cast<bool>();
        if (ImGui::Checkbox(memberName, &value))
        {
            metaData.set(componentInstance, value);
            modified = true;
        }
    }
    else if (memberType == entt::resolve<std::string>())
    {
        auto value = getter.cast<std::string>();
        char buffer[256];
        strcpy_s(buffer, sizeof(buffer), value.c_str());
        if (ImGui::InputText(memberName, buffer, sizeof(buffer)))
        {
            metaData.set(componentInstance, std::string(buffer));
            modified = true;
        }
    }
    else if (memberType == entt::resolve<float>())
    {
        auto value = getter.cast<float>();
        if (ImGui::DragFloat(memberName, &value, 0.1f))
        {
            metaData.set(componentInstance, value);
            modified = true;
        }
    }
    else if (memberType == entt::resolve<int>())
    {
        auto value = getter.cast<int>();
        if (ImGui::DragInt(memberName, &value))
        {
            metaData.set(componentInstance, value);
            modified = true;
        }
    }
    else
    {
        // 对于不支持的类型，仅显示信息
        ImGui::Text("%s: %s", memberName, memberType.info().name());
    }

    return modified;
}

static void DrawComponent(entt::meta_type &metaType, entt::meta_any &componentInstance)
{
    // 获取友好的组件名称
    const char *componentName = metaType.name();

    if (ImGui::CollapsingHeader(componentName, ImGuiTreeNodeFlags_DefaultOpen))
    {
        // 现在可以遍历所有数据成员
        for (auto &&[data_id, meta_data] : metaType.data())
        {
            DrawDataMember(meta_data, componentInstance);
        }
    }
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

    for (auto &&[id, storage] : m_Context->storage())
    {
        if (storage.contains(selectedEntity))
        {
            auto metaType = entt::resolve(storage.info());
            if (!metaType)
                continue;

            // 获取组件的原始指针
            void *componentPtr = storage.value(selectedEntity);

            // 用 meta_any 包装组件实例，这样反射系统就能操作它
            entt::meta_any componentInstance = metaType.from_void(componentPtr);

            // draw component name
            DrawComponent(metaType, componentInstance);
        }
    }
    ImGui::End();
}