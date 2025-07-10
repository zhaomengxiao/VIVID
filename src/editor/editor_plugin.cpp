#include "editor_plugin.h"
#include "app/App.h"

// System implementations for UI
namespace
{

    void ui_startup_system(Resources &res, entt::registry &world)
    {
        // Simplified startup system without ImGui
        std::cout << "Editor Plugin: UI Startup System" << std::endl;
    }

    void ui_update_system(Resources &res, entt::registry &world)
    {
        // Simplified update system without ImGui
        // Just update viewport components
        world.view<ViewportComponent, CameraComponent>().each(
            [](auto entity, ViewportComponent &viewport, CameraComponent & /* camera */)
            {
                // Set default viewport size for testing
                viewport.Width = 1280.0f;
                viewport.Height = 720.0f;
            });
    }

    void ui_shutdown_system(Resources &, entt::registry &)
    {
        // Simplified shutdown system without ImGui
        std::cout << "Editor Plugin: UI Shutdown System" << std::endl;
    }

} // anonymous namespace

void EditorPlugin::build(App &app)
{
    app.add_system(ScheduleLabel::Startup, ui_startup_system);
    app.add_system(ScheduleLabel::Update, ui_update_system);
    app.add_system(ScheduleLabel::Shutdown, ui_shutdown_system);
}

std::string EditorPlugin::name() const
{
    return "EditorPlugin";
}