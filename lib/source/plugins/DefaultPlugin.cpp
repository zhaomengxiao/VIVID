#include "vivid/app/App.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "vivid/input/input_system.h"
#include <vivid/plugins/DefaultPlugin.h>

namespace
{

    void window_init_system(App &app, Resources &res)
    {
        if (!glfwInit())
        {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            exit(-1);
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

        GLFWmonitor *primary = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(primary);

        GLFWwindow *window = glfwCreateWindow(mode->width, mode->height, "VIVID ECS Renderer", nullptr, nullptr);
        if (!window)
        {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            exit(-1);
        }
        glfwMakeContextCurrent(window);

        if (glewInit() != GLEW_OK)
        {
            std::cerr << "Failed to initialize GLEW" << std::endl;
            exit(-1);
        }

        std::cout << "Using GL Version: " << glGetString(GL_VERSION) << std::endl;
        glEnable(GL_DEPTH_TEST);

        res.insert<WindowResource>(WindowResource{window});
        
        // Initialize input system
        InputSystem::Initialize(window);
    }

    void window_close_system(App &app, Resources &res)
    {
        auto *windowRes = res.get<WindowResource>();
        if (windowRes && glfwWindowShouldClose(windowRes->window))
        {
            app.exit();
        }
    }

    void window_update_system(Resources &res, entt::registry &registry)
    {
        auto *windowRes = res.get<WindowResource>();
        if (windowRes)
        {
            // Set registry for input system callbacks
            glfwSetWindowUserPointer(windowRes->window, &registry);
            
            // Update input system
            InputSystem::Update(registry);
            
            glfwSwapBuffers(windowRes->window);
            glfwPollEvents();
        }
    }

    void window_shutdown_system(Resources &res, entt::registry &)
    {
        // Shutdown input system
        InputSystem::Shutdown();
        
        auto *windowRes = res.get<WindowResource>();
        if (windowRes)
        {
            glfwDestroyWindow(windowRes->window);
        }
        glfwTerminate();
        std::cout << "GLFW resources cleaned up." << std::endl;
    }

} // anonymous namespace

void DefaultPlugin::build(App &app)
{
    // Note: The main initialization is not a typical ECS system,
    // as it needs to run before any other system and register resources.
    // We call it directly here.
    window_init_system(app, app.resources());

    // Add other systems to the schedule
    app.add_system(ScheduleLabel::PreUpdate, [this, &app](Resources &res, entt::registry &)
                   { window_close_system(app, res); });
    app.add_system(ScheduleLabel::PostUpdate, window_update_system);
    app.add_system(ScheduleLabel::Shutdown, window_shutdown_system);
}

std::string DefaultPlugin::name() const
{
    return "DefaultPlugin";
}