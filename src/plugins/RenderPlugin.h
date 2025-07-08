#pragma once

#include "core/App.h"
#include "core/Plugin.h"
#include "systems/renderer_system.h"
#include "editor/ComponentRegistry.h"
#include "components/rendering_components.h"
#include <vector>

class RenderPlugin : public Plugin
{
public:
    void build(App &app) override;
    std::string name() const override;
};