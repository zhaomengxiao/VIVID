#include "InspectorPanel.h"
#include <vivid/rendering/render_component.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <iostream> // 引入头文件

namespace
{
    enum class UITrait : int
    {
        None = 0,
        AsColor = 1 << 0, // 标志1：把它当作颜色
        AsSlider = 1 << 1 // 标志2：未来可以扩展，比如把它当作滑块
        // ... 其他未来可能的标志
    };
    // Constants
    constexpr size_t INPUT_BUFFER_SIZE = 256;
    constexpr float DEFAULT_DRAG_SPEED = 0.1f;
    constexpr float LABEL_COLUMN_WIDTH_SCALE = 7.0f;

    using namespace entt::literals;

    // Get the default reset value for a field
    float GetResetValue(entt::id_type data_id)
    {
        return (data_id == "Scale"_hs) ? 1.0f : 0.0f;
    }

    // Improved Vec3 control with error handling
    bool DrawVec3Control(const std::string &label, glm::vec3 &values, float resetValue = 0.0f)
    {
        if (label.empty())
            return false;

        bool modified = false;

        ImGui::PushID(label.c_str());
        ImGuiIO &io = ImGui::GetIO();

        // Use a dynamic column width based on font size
        float columnWidth = ImGui::GetFontSize() * io.FontGlobalScale * LABEL_COLUMN_WIDTH_SCALE;
        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text("%s", label.c_str()); // Prevent format string exploits
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

        float lineHeight = GImGui->Font->FontSize * io.FontGlobalScale + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = {lineHeight + GImGui->Style.FramePadding.x, lineHeight};

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
        auto user_traits = metaData.traits<UITrait>();

        // Type-driven UI generation
        if (memberType == entt::resolve<glm::vec3>())
        {
            auto vec3 = getter.cast<glm::vec3>();
            auto user_traits = metaData.traits<UITrait>();

            // 检查是否存在 AsColor 特性
            if (static_cast<std::underlying_type_t<UITrait>>(user_traits) & static_cast<std::underlying_type_t<UITrait>>(UITrait::AsColor))
            {
                // --- 开始自定义颜色控件布局 ---

                // 使用成员变量名作为唯一ID
                ImGui::PushID(memberName);

                // 创建两列
                ImGui::Columns(2);

                // 设置第一列（标签列）的宽度
                float columnWidth = ImGui::GetFontSize() * ImGui::GetIO().FontGlobalScale * LABEL_COLUMN_WIDTH_SCALE;
                ImGui::SetColumnWidth(0, columnWidth);

                // 在第一列绘制标签
                ImGui::Text("%s", memberName);

                // 切换到第二列
                ImGui::NextColumn();

                // 在第二列绘制颜色控件，使用 "##" 隐藏其自带标签
                if (ImGui::ColorEdit3("##Color", &vec3.x))
                {
                    if (metaData.set(componentInstance, vec3))
                    {
                        modified = true;
                    }
                }

                // 恢复单列布局
                ImGui::Columns(1);
                ImGui::PopID();

                // --- 布局结束 ---
            }
            else
            {
                // 否则，渲染XYZ控制器
                float resetVal = GetResetValue(data_id);
                if (DrawVec3Control(memberName, vec3, resetVal))
                {
                    if (metaData.set(componentInstance, vec3))
                    {
                        modified = true;
                    }
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
            // Use a dynamic column width
            ImGui::SetColumnWidth(0, ImGui::GetFontSize() * ImGui::GetIO().FontGlobalScale * LABEL_COLUMN_WIDTH_SCALE);
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

            ImGui::PushID(memberName);
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, ImGui::GetFontSize() * ImGui::GetIO().FontGlobalScale * LABEL_COLUMN_WIDTH_SCALE);
            ImGui::Text("%s", memberName);
            ImGui::NextColumn();

            if (ImGui::DragFloat("##Value", &value, DEFAULT_DRAG_SPEED))
            {
                if (metaData.set(componentInstance, value))
                {
                    modified = true;
                }
            }

            ImGui::Columns(1);
            ImGui::PopID();
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
        else if (memberType == entt::resolve<std::vector<float>>())
        {
            auto value = getter.cast<std::vector<float>>();
            ImGui::Text("%s: %zu", memberName, value.size());
        }
        else if (memberType == entt::resolve<std::vector<unsigned int>>())
        {
            auto value = getter.cast<std::vector<unsigned int>>();
            ImGui::Text("%s: %zu", memberName, value.size());
        }
        else if (memberType == entt::resolve<size_t>())
        {
            auto value = getter.cast<size_t>();
            ImGui::Text("%s: %zu", memberName, value);
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

    // Helper function to create a cube mesh component
    MeshComponent CreateCubeMesh()
    {
        std::vector<float> vertices = {
            // positions          // normals
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
            0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,

            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,

            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
            -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f,

            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
            0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,

            -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
            0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,

            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f};
        std::vector<unsigned int> indices = {
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4,
            8, 9, 10, 10, 11, 8,
            12, 13, 14, 14, 15, 12,
            16, 17, 18, 18, 19, 16,
            20, 21, 22, 22, 23, 20};

        return {vertices, indices, indices.size()};
    }

    void DrawMeshComponent(entt::registry &registry, entt::entity entity)
    {
        if (ImGui::CollapsingHeader("MeshComponent", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &meshComponent = registry.get<MeshComponent>(entity);

            // Display some info
            ImGui::Text("Vertex Count: %zu", meshComponent.m_Vertices.size() / 6); // 6 floats per vertex (pos+normal)
            ImGui::Text("Index Count: %zu", meshComponent.m_IndexCount);

            ImGui::Spacing();

            // Button to generate cube
            if (ImGui::Button("Generate Cube"))
            {
                registry.replace<MeshComponent>(entity, CreateCubeMesh());
            }
        }
    }
} // namespace

void InspectorPanel::DrawAddComponentButton(entt::entity selectedEntity)
{
    ImGui::Separator();

    // Add some vertical spacing
    ImGui::Spacing();
    ImGui::Spacing();

    // Center the button
    float buttonWidth = ImGui::GetContentRegionAvail().x * 0.8f;
    float buttonPosX = (ImGui::GetContentRegionAvail().x - buttonWidth) * 0.5f;
    ImGui::SetCursorPosX(buttonPosX);

    if (ImGui::Button("Add Component", {buttonWidth, 0}))
    {
        ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        for (auto [id, type] : entt::resolve())
        {
            if (auto emplace_func = type.func("emplace_func"_hs); emplace_func)
            {
                auto *storage = m_Context->storage(id);
                if (storage && !storage->contains(selectedEntity))
                {
                    if (ImGui::MenuItem(type.name()))
                    {
                        // 添加日志，打印将要添加的组件名称和实体ID
                        std::cout << "[LOG] MenuItem clicked for: " << type.name()
                                  << ", on entity: " << static_cast<uint32_t>(selectedEntity) << std::endl;

                        emplace_func.invoke({}, std::ref(*m_Context), selectedEntity);
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
        }

        ImGui::EndPopup();
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

    // Check entity validity
    if (selectedEntity == entt::null || !m_Context || !m_Context->valid(selectedEntity))
    {
        ImGui::Text("Please select a valid entity");
        ImGui::End();
        return;
    }

    // Iterate through all storages to find components owned by the entity
    bool hasComponents = false;
    for (auto &&[id, pool] : m_Context->storage())
    {
        if (!pool.contains(selectedEntity))
            continue;

        hasComponents = true;
        auto metaType = entt::resolve(pool.info());
        if (!metaType)
            continue;

        // Get the raw component pointer and wrap it as a meta_any
        void *componentPtr = pool.value(selectedEntity);
        if (!componentPtr)
            continue;

        entt::meta_any componentInstance = metaType.from_void(componentPtr);
        if (!componentInstance)
            continue;

        if (metaType.id() == "MeshComponent"_hs)
        {
            DrawMeshComponent(*m_Context, selectedEntity);
        }
        else
        {
            DrawComponent(metaType, componentInstance);
        }
    }

    if (!hasComponents)
    {
        ImGui::Text("This entity has no components");
    }

    DrawAddComponentButton(selectedEntity);

    ImGui::End();
}