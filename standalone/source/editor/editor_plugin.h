#pragma once

#include <vivid/app/App.h>
#include <vivid/app/Plugin.h>
#include "SceneHierarchyPanel.h"
#include "InspectorPanel.h"
#include <vivid/plugins/DefaultPlugin.h> // For WindowResource
#include <vivid/rendering/render_component.h>

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