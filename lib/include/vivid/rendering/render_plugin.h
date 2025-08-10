#pragma once
#include <vivid/app/App.h>

class RenderPlugin : public Plugin
{
public:
    void build(App &app) override;
    std::string name() const override;
};