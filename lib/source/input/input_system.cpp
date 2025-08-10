#include "vivid/input/input_system.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "vivid/rendering/render_component.h"

GLFWwindow *InputSystem::s_Window = nullptr;
glm::vec2 InputSystem::s_MousePosition = glm::vec2(0.0f);
glm::vec2 InputSystem::s_LastMousePosition = glm::vec2(0.0f);
bool InputSystem::s_MousePressed = false;
bool InputSystem::s_FirstMouse = true;

void InputSystem::Initialize(GLFWwindow *window)
{
    s_Window = window;
    glfwSetCursorPosCallback(window, MousePositionCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetKeyCallback(window, KeyCallback);

    // Initially show cursor (don't capture it)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void InputSystem::Update(entt::registry &registry)
{
    if (!s_Window)
        return;

    // Find the main camera with camera controller
    auto cameraView = registry.view<TransformComponent, CameraComponent, CameraControllerComponent>();
    if (cameraView.size_hint() == 0)
        return;

    auto cameraEntity = cameraView.front();
    auto &transform = cameraView.get<TransformComponent>(cameraEntity);
    auto &cameraController = cameraView.get<CameraControllerComponent>(cameraEntity);

    // Handle keyboard movement
    if (glfwGetKey(s_Window, GLFW_KEY_W) == GLFW_PRESS)
        transform.Position += cameraController.Front * cameraController.MovementSpeed * 0.016f; // Assuming 60 FPS
    if (glfwGetKey(s_Window, GLFW_KEY_S) == GLFW_PRESS)
        transform.Position -= cameraController.Front * cameraController.MovementSpeed * 0.016f;
    if (glfwGetKey(s_Window, GLFW_KEY_A) == GLFW_PRESS)
        transform.Position -= cameraController.Right * cameraController.MovementSpeed * 0.016f;
    if (glfwGetKey(s_Window, GLFW_KEY_D) == GLFW_PRESS)
        transform.Position += cameraController.Right * cameraController.MovementSpeed * 0.016f;
    if (glfwGetKey(s_Window, GLFW_KEY_Q) == GLFW_PRESS)
        transform.Position += cameraController.Up * cameraController.MovementSpeed * 0.016f;
    if (glfwGetKey(s_Window, GLFW_KEY_E) == GLFW_PRESS)
        transform.Position -= cameraController.Up * cameraController.MovementSpeed * 0.016f;

    // Update camera vectors based on current yaw and pitch
    cameraController.UpdateVectors();

    // Update rotation from camera controller
    transform.Rotation.x = cameraController.Pitch;
    transform.Rotation.y = cameraController.Yaw;
    transform.Rotation.z = 0.0f;
}

void InputSystem::Shutdown()
{
    if (s_Window)
    {
        glfwSetCursorPosCallback(s_Window, nullptr);
        glfwSetMouseButtonCallback(s_Window, nullptr);
        glfwSetScrollCallback(s_Window, nullptr);
        glfwSetKeyCallback(s_Window, nullptr);
    }
}

void InputSystem::MousePositionCallback(GLFWwindow *window, double xpos, double ypos)
{
    if (s_FirstMouse)
    {
        s_LastMousePosition.x = static_cast<float>(xpos);
        s_LastMousePosition.y = static_cast<float>(ypos);
        s_FirstMouse = false;
        return;
    }

    float xoffset = static_cast<float>(xpos) - s_LastMousePosition.x;
    float yoffset = s_LastMousePosition.y - static_cast<float>(ypos); // Reversed since y-coordinates go from bottom to top

    s_LastMousePosition.x = static_cast<float>(xpos);
    s_LastMousePosition.y = static_cast<float>(ypos);

    if (!s_Window)
        return;

    // Get the entt registry from window user pointer
    entt::registry *registry = static_cast<entt::registry *>(glfwGetWindowUserPointer(window));
    if (!registry)
        return;

    // Find camera with controller
    auto cameraView = registry->view<TransformComponent, CameraComponent, CameraControllerComponent>();
    if (cameraView.size_hint() == 0)
        return;

    auto cameraEntity = cameraView.front();
    auto &cameraController = cameraView.get<CameraControllerComponent>(cameraEntity);

    // Only process mouse movement if mouse is pressed (dragging)
    if (cameraController.MousePressed && cameraController.IsActive)
    {
        xoffset *= cameraController.MouseSensitivity;
        yoffset *= cameraController.MouseSensitivity;

        cameraController.Yaw += xoffset;
        cameraController.Pitch += yoffset;

        // Constrain pitch
        if (cameraController.Pitch > 89.0f)
            cameraController.Pitch = 89.0f;
        if (cameraController.Pitch < -89.0f)
            cameraController.Pitch = -89.0f;
    }
}

void InputSystem::MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        entt::registry *registry = static_cast<entt::registry *>(glfwGetWindowUserPointer(window));
        if (!registry)
            return;

        // Find camera with controller
        auto cameraView = registry->view<TransformComponent, CameraComponent, CameraControllerComponent, ViewportComponent>();
        if (cameraView.size_hint() == 0)
            return;

        auto cameraEntity = cameraView.front();
        auto &cameraController = cameraView.get<CameraControllerComponent>(cameraEntity);
        
        if (action == GLFW_PRESS)
        {
            cameraController.MousePressed = true;
            cameraController.IsActive = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Hide cursor when dragging
        }
        else if (action == GLFW_RELEASE)
        {
            cameraController.MousePressed = false;
            cameraController.IsActive = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Show cursor when released
        }
    }
}

void InputSystem::ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    if (!s_Window)
        return;

    entt::registry *registry = static_cast<entt::registry *>(glfwGetWindowUserPointer(window));
    if (!registry)
        return;

    auto cameraView = registry->view<TransformComponent, CameraComponent, CameraControllerComponent, ViewportComponent>();
    if (cameraView.size_hint() == 0)
        return;

    auto cameraEntity = cameraView.front();
    auto &transform = cameraView.get<TransformComponent>(cameraEntity);
    auto &cameraController = cameraView.get<CameraControllerComponent>(cameraEntity);

    // Zoom works without needing to press mouse button, just when hovering
    float zoomAmount = static_cast<float>(yoffset) * cameraController.ZoomSpeed;
    transform.Position += cameraController.Front * zoomAmount;
}

void InputSystem::KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    // Handle special keys like ESC to toggle cursor capture
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        static bool cursorCaptured = true;
        if (cursorCaptured)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        cursorCaptured = !cursorCaptured;
    }
}