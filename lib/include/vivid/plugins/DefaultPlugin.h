#pragma once


#include <GLFW/glfw3.h>

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