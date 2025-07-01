#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <vector>

#include <entt/entt.hpp>
#include "components/rendering_components.h"
#include "systems/renderer_system.h"
#include "editor/SceneHierarchyPanel.h"
#include "editor/InspectorPanel.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

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

void RegisterComponents()
{
    using namespace entt;
    using namespace entt::literals;

    meta_factory<TagComponent>()
        .type("TagComponent")
        .data<&TagComponent::Tag>("Tag");

    meta_factory<TransformComponent>()
        .type("TransformComponent")
        .data<&TransformComponent::Position>("Position")
        .data<&TransformComponent::Rotation>("Rotation")
        .data<&TransformComponent::Scale>("Scale");

    meta_factory<CameraComponent>()
        .type("CameraComponent")
        .data<&CameraComponent::IsPrimary>("IsPrimary");
}

int main()
{
    // --- Register Components for reflection ---
    RegisterComponents();

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

    // --- ImGui Setup ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

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
    registry.emplace<CameraComponent>(cameraEntity);
    registry.emplace<ViewportComponent>(cameraEntity);

    // --- Scene Hierarchy Panel ---
    SceneHierarchyPanel sceneHierarchyPanel;
    sceneHierarchyPanel.SetContext(&registry);

    // --- Inspector Panel ---
    InspectorPanel inspectorPanel;
    inspectorPanel.SetContext(&registry);

    // --- Renderer System Init ---
    VIVID::RendererSystem::Init();
    // --- Sync ECS data to GPU ---
    // This needs to be called once after entities are set up,
    // to upload mesh data and compile shaders.
    VIVID::RendererSystem::Sync(registry);

    // --- Main Loop ---
    while (!glfwWindowShouldClose(window))
    {
        // Input processing
        // ...

        // --- ImGui Frame Start ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport();

        sceneHierarchyPanel.OnImGuiRender();

        auto selectedEntity = sceneHierarchyPanel.GetSelectedEntity();
        inspectorPanel.OnImGuiRender(selectedEntity);

        // --- Scene Viewport ---
        ImGui::Begin("Viewport");

        auto &viewport = registry.get<ViewportComponent>(cameraEntity);
        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        viewport.Width = viewportPanelSize.x;
        viewport.Height = viewportPanelSize.y;

        uint32_t textureID = viewport.TextureID;
        ImGui::Image((ImTextureID)(intptr_t)textureID, ImVec2(viewport.Width, viewport.Height), ImVec2(0, 1), ImVec2(1, 0));

        ImGui::End();

        // Call our renderer system
        VIVID::RendererSystem::Update(registry);

        // --- ImGui Render ---
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- Cleanup ---
    VIVID::RendererSystem::Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
