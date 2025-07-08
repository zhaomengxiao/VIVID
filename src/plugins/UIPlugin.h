#pragma once

#include "core/App.h"
#include "core/Plugin.h"
#include "editor/SceneHierarchyPanel.h"
#include "editor/InspectorPanel.h"
#include "plugins/DefaultPlugin.h" // For WindowResource
#include "components/rendering_components.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>

class UIPlugin : public Plugin
{
public:
    void build(App &app) override;
    std::string name() const override;
};