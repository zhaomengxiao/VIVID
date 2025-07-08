#pragma once

#include "core/App.h"
#include "core/Plugin.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

// A resource to hold the GLFW window pointer, managed by the DefaultPlugin.
struct WindowResource
{
    GLFWwindow *window;
};

class DefaultPlugin : public Plugin
{
public:
    void build(App &app) override;
    std::string name() const override;
};