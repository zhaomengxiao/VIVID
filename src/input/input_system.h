#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <entt/entt.hpp>

#include "../rendering/render_component.h"
#include "camera_controller.h"

class InputSystem
{
public:
    static void Initialize(GLFWwindow* window);
    static void Update(entt::registry& registry);
    static void Shutdown();
    
    static void SetWindow(GLFWwindow* window) { s_Window = window; }
    
private:
    static void MousePositionCallback(GLFWwindow* window, double xpos, double ypos);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    
    static GLFWwindow* s_Window;
    static glm::vec2 s_MousePosition;
    static glm::vec2 s_LastMousePosition;
    static bool s_MousePressed;
    static bool s_FirstMouse;
};