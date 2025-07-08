#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <vector>

#include <entt/entt.hpp>
#include "components/rendering_components.h"
#include "systems/renderer_system.h"
#include "editor/SceneHierarchyPanel.h"
#include "editor/InspectorPanel.h"
#include "editor/ComponentRegistry.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "core/App.h"
#include "plugins/DefaultPlugin.h"
#include "plugins/UIPlugin.h"
#include "plugins/RenderPlugin.h"

int main()
{
    auto &app = App::new_app();

    app.add_plugin<DefaultPlugin>()
        .add_plugin<RenderPlugin>()
        .add_plugin<UIPlugin>();

    app.run();

    return 0;
}
