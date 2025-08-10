#include "editor/ComponentRegistry.h"
#include <vivid/rendering/render_component.h>
#include <entt/entt.hpp>
#include <iostream>
#include <functional>

namespace VIVID
{
    enum class UITrait
    {
        None = 0,
        AsColor = 1 << 0, // 标志1：把它当作颜色
        AsSlider = 1 << 1 // 标志2：未来可以扩展，比如把它当作滑块
        // ... 其他未来可能的标志
    };

    template <typename T>
    void emplaceComponent(std::reference_wrapper<entt::registry> registry_wrapper, entt::entity entity)
    {
        // 在这里添加日志！
        std::cout << "[LOG] Emplace function invoked for component: "
                  << entt::type_id<T>().name() // 使用 type_id 获取组件名
                  << " on entity " << static_cast<uint32_t>(entity) << std::endl;

        entt::registry &registry = registry_wrapper.get();

        if (!registry.any_of<T>(entity))
        {
            registry.emplace<T>(entity);
            std::cout << "[LOG] Component emplaced successfully" << std::endl;
        }
        else
        {
            std::cout << "[LOG] Component already exists" << std::endl;
        }
    }

    template <typename Component>
    entt::meta_any get_for_entity(entt::registry &registry, entt::entity entity)
    {
        return entt::forward_as_meta(registry.get<Component>(entity));
    }

    void ComponentRegistry::RegisterAllComponents()
    {
        using namespace entt;

        // 新增：为所有组件注册的通用 lambda

        // Registering components using a cleaner, more organized approach.
        meta_factory<TagComponent>()
            .type("TagComponent")
            .data<&TagComponent::Tag>("Tag")
            .func<&get_for_entity<TagComponent>>("get");

        meta_factory<TransformComponent>()
            .type("TransformComponent")
            .data<&TransformComponent::Position>("Position")
            .data<&TransformComponent::Rotation>("Rotation")
            .data<&TransformComponent::Scale>("Scale")
            .func<&emplaceComponent<TransformComponent>>("emplace_func")
            .func<&get_for_entity<TransformComponent>>("get");

        meta_factory<MeshComponent>()
            .type("MeshComponent")
            .data<&MeshComponent::m_Vertices>("Vertices")
            .data<&MeshComponent::m_Indices>("Indices")
            .data<&MeshComponent::m_IndexCount>("IndexCount")
            .func<&emplaceComponent<MeshComponent>>("emplace_func")
            .func<&get_for_entity<MeshComponent>>("get");

        meta_factory<MaterialComponent>()
            .type("MaterialComponent")
            .data<&MaterialComponent::ShaderPath>("ShaderPath")
            .data<&MaterialComponent::ObjectColor>("ObjectColor")
            .traits(UITrait::AsColor)
            .data<&MaterialComponent::SpecularColor>("SpecularColor")
            .traits(UITrait::AsColor)
            .data<&MaterialComponent::Shininess>("Shininess")
            .func<&emplaceComponent<MaterialComponent>>("emplace_func")
            .func<&get_for_entity<MaterialComponent>>("get");

        meta_factory<LightComponent>()
            .type("LightComponent")
            .data<&LightComponent::LightColor>("LightColor")
            .data<&LightComponent::AmbientColor>("AmbientColor")
            .data<&LightComponent::Constant>("Constant")
            .data<&LightComponent::Linear>("Linear")
            .data<&LightComponent::Quadratic>("Quadratic")
            .func<&emplaceComponent<LightComponent>>("emplace_func")
            .func<&get_for_entity<LightComponent>>("get");

        meta_factory<CameraComponent>()
            .type("CameraComponent")
            .data<&CameraComponent::IsPrimary>("IsPrimary")
            .func<&emplaceComponent<CameraComponent>>("emplace_func")
            .func<&get_for_entity<CameraComponent>>("get");
    }
}