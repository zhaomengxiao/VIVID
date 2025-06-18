#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <vector>

#include <entt/entt.hpp>
#include "components/rendering_components.h"
#include "systems/renderer_system.h"

// The RendererSystem now handles all OpenGL details.
// We no longer need to include these low-level files here.
// #include "opengl/buffer/VertexBuffer.h"
// #include "opengl/buffer/VertexBufferLayout.h"
// #include "opengl/buffer/IndexBuffer.h"
// #include "opengl/buffer/VertexArray.h"
// #include "opengl/shader/Shader.h"
#include "opengl/GLErrorHandler.h"

// Helper function to create a cube mesh component
MeshComponent CreateCubeMesh()
{
    std::vector<float> vertices = {
        // positions          // normals
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
        0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,

        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,

        -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f,

        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,

        -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,

        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f};
    std::vector<unsigned int> indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20};

    return {vertices, indices, indices.size()};
}

int main()
{
    // --- GLFW/GLEW Initialization ---
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(800, 600, "VIVID ECS Renderer", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    std::cout << "Using GL Version: " << glGetString(GL_VERSION) << std::endl;

    glEnable(GL_DEPTH_TEST);

    // --- ECS Setup ---
    entt::registry registry;

    // Create a renderable cube entity
    auto cubeEntity = registry.create();
    registry.emplace<TagComponent>(cubeEntity, "MyCube");
    registry.emplace<TransformComponent>(cubeEntity);
    registry.emplace<MeshComponent>(cubeEntity, CreateCubeMesh());

    // NOTE: MaterialComponent now takes a shader path and other material properties.
    // The RendererSystem will handle loading the shader from the path.
    // By creating a temporary MaterialComponent with {}, we ensure compatibility
    // across different C++ standards.
    registry.emplace<MaterialComponent>(cubeEntity, MaterialComponent{
                                                        "res/shaders/BlinnPhong.shader", // ShaderPath
                                                        {1.0f, 0.5f, 0.2f}               // ObjectColor
                                                    });

    // Create a light entity
    auto lightEntity = registry.create();
    registry.emplace<TagComponent>(lightEntity, "PointLight");
    auto &lightTransform = registry.emplace<TransformComponent>(lightEntity);
    lightTransform.Position = {1.2f, 1.0f, 2.0f};
    registry.emplace<LightComponent>(lightEntity);

    // Create a camera entity
    auto cameraEntity = registry.create();
    registry.emplace<TagComponent>(cameraEntity, "MainCamera");
    auto &camTransform = registry.emplace<TransformComponent>(cameraEntity);
    camTransform.Position = {0.0f, 0.0f, 5.0f};
    auto &camComponent = registry.emplace<CameraComponent>(cameraEntity);
    camComponent.ProjectionMatrix = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    // --- Sync ECS data to GPU ---
    // This needs to be called once after entities are set up,
    // to upload mesh data and compile shaders.
    VIVID::RendererSystem::Sync(registry);

    // --- Main Loop ---
    while (!glfwWindowShouldClose(window))
    {
        // Input processing
        // ...

        // Update cube rotation for some animation
        auto &cubeTransform = registry.get<TransformComponent>(cubeEntity);
        cubeTransform.Rotation.y += 0.01f;
        cubeTransform.Rotation.x += 0.005f;

        // The RendererSystem's Update function now handles clearing the screen
        // and drawing all renderable entities.
        // glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Call our renderer system
        VIVID::RendererSystem::Update(registry);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- Cleanup ---
    // EnTT registry handles component destruction automatically.
    // Smart pointers for Shader/VAO will clean up GL resources.
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
