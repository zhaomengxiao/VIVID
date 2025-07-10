#pragma once

#include "app/App.h"
#include "app/Plugin.h"
#include "SceneHierarchyPanel.h"
#include "InspectorPanel.h"
#include "plugins/DefaultPlugin.h" // For WindowResource
#include "rendering/render_component.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>

class EditorPlugin : public Plugin
{
public:
    void build(App &app) override;
    std::string name() const override;
};