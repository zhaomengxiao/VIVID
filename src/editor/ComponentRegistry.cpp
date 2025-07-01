#include "editor/ComponentRegistry.h"
#include "components/rendering_components.h"
#include <entt/meta/factory.hpp>
#include <entt/meta/resolve.hpp>

namespace VIVID
{
    void ComponentRegistry::RegisterAllComponents()
    {
        using namespace entt;

        // Registering components using a cleaner, more organized approach.
        meta_factory<TagComponent>()
            .type("TagComponent")
            .data<&TagComponent::Tag>("Tag");

        meta_factory<TransformComponent>()
            .type("TransformComponent")
            .data<&TransformComponent::Position>("Position")
            .data<&TransformComponent::Rotation>("Rotation")
            .data<&TransformComponent::Scale>("Scale");

        meta_factory<MaterialComponent>()
            .type("MaterialComponent")
            .data<&MaterialComponent::ShaderPath>("ShaderPath")
            .data<&MaterialComponent::ObjectColor>("ObjectColor")
            .data<&MaterialComponent::SpecularColor>("SpecularColor")
            .data<&MaterialComponent::Shininess>("Shininess");

        meta_factory<LightComponent>()
            .type("LightComponent")
            .data<&LightComponent::LightColor>("LightColor")
            .data<&LightComponent::AmbientColor>("AmbientColor")
            .data<&LightComponent::Constant>("Constant")
            .data<&LightComponent::Linear>("Linear")
            .data<&LightComponent::Quadratic>("Quadratic");

        meta_factory<CameraComponent>()
            .type("CameraComponent")
            .data<&CameraComponent::IsPrimary>("IsPrimary");
    }
}