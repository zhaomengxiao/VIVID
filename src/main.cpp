#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <vector>

#include <entt/entt.hpp>
#include "rendering/render_component.h"
#include "rendering/render_system.h"
#include "editor/SceneHierarchyPanel.h"
#include "editor/InspectorPanel.h"
#include "editor/ComponentRegistry.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "app/App.h"
#include "plugins/DefaultPlugin.h"
#include "editor/editor_plugin.h"
#include "rendering/render_plugin.h"

int main()
{
    auto &app = App::new_app();

    app.add_plugin<DefaultPlugin>()
        .add_plugin<RenderPlugin>()
        .add_plugin<EditorPlugin>();

    app.run();

    return 0;
}
