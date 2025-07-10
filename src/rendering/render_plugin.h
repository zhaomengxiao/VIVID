#pragma once

#include "app/App.h"
#include "app/Plugin.h"
#include "render_system.h"
#include "editor/ComponentRegistry.h"
#include "render_component.h"
#include <vector>

class RenderPlugin : public Plugin
{
public:
    void build(App &app) override;
    std::string name() const override;
};