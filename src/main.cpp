/*******************************************************************************************
 *
 *   raylib-extras [ImGui] example - Docking example
 *
 *	This is an example of using the ImGui docking features that are part of docking branch
 *	You must replace the default imgui with the code from the docking branch for this to work
 *	https://github.com/ocornut/imgui/tree/docking
 *
 *   Copyright (c) 2024 Jeffery Myers
 *
 ********************************************************************************************/

#include "raylib.h"
#include "raymath.h"

#include "imgui.h"
#include "rlImGui.h"
#include "entt/entt.hpp"
#include "PxPhysicsAPI.h"

// DPI scaling functions
float ScaleToDPIF(float value)
{
    return GetWindowScaleDPI().x * value;
}

int ScaleToDPII(int value)
{
    return int(GetWindowScaleDPI().x * value);
}

int main(int argc, char *argv[])
{
    // PhysX 初始化
    static physx::PxDefaultAllocator gAllocator;
    static physx::PxDefaultErrorCallback gErrorCallback;
    physx::PxFoundation *foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
    physx::PxPhysics *physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, physx::PxTolerancesScale());
    physx::PxSceneDesc sceneDesc(physics->getTolerancesScale());
    sceneDesc.gravity = physx::PxVec3(0.0f, -9.8f, 0.0f);
    physx::PxDefaultCpuDispatcher *dispatcher = physx::PxDefaultCpuDispatcherCreate(2);
    sceneDesc.cpuDispatcher = dispatcher;
    sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
    physx::PxScene *scene = physics->createScene(sceneDesc);

    // 创建一个球体刚体
    physx::PxMaterial *material = physics->createMaterial(0.5f, 0.5f, 0.6f);
    physx::PxShape *shape = physics->createShape(physx::PxSphereGeometry(1.0f), *material);
    physx::PxTransform startTransform(physx::PxVec3(0.0f, 10.0f, 0.0f));
    physx::PxRigidDynamic *dynamic = physics->createRigidDynamic(startTransform);
    dynamic->attachShape(*shape);
    physx::PxRigidBodyExt::updateMassAndInertia(*dynamic, 1.0f);
    scene->addActor(*dynamic);

    // Initialization
    //--------------------------------------------------------------------------------------
    int screenWidth = 1280;
    int screenHeight = 800;

    // do not set the FLAG_WINDOW_HIGHDPI flag, that scales a low res framebuffer up to the native resolution.
    // use the native resolution and scale your geometry.
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "raylib-Extras [ImGui] example - Docking");
    SetTargetFPS(30);
    rlImGuiSetup(true);

    bool run = true;

    bool showDemoWindow = true;

    // if the linked ImGui has docking, enable it.
    // this will only be true if you use the docking branch of ImGui.
#ifdef IMGUI_HAS_DOCK
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif

    // Main game loop
    while (!WindowShouldClose() && run) // Detect window close button or ESC key, or a quit from the menu
    {
        BeginDrawing();
        ClearBackground(DARKGRAY);

        // draw something to the raylib window below the GUI.
        DrawCircle(GetScreenWidth() / 2, GetScreenHeight() / 2, GetScreenHeight() * 0.45f, DARKGREEN);

        // start ImGui content
        rlImGuiBegin();

        // if you want windows to dock to the viewport, call this.
#ifdef IMGUI_HAS_DOCK
        ImGui::DockSpaceOverViewport(0, NULL, ImGuiDockNodeFlags_PassthruCentralNode); // set ImGuiDockNodeFlags_PassthruCentralNode so that we can see the raylib contents behind the dockspace
#endif

        // show a simple menu bar
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Quit"))
                    run = false;

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Window"))
            {
                if (ImGui::MenuItem("Demo Window", nullptr, showDemoWindow))
                    showDemoWindow = !showDemoWindow;

                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // show some windows

        if (showDemoWindow)
            ImGui::ShowDemoWindow(&showDemoWindow);

        if (ImGui::Begin("Test Window"))
        {
            // 获取球体刚体的位置
            physx::PxTransform pose = dynamic->getGlobalPose();
            ImGui::Text("PhysX 球体位置: x=%.2f, y=%.2f, z=%.2f", pose.p.x, pose.p.y, pose.p.z);
        }
        ImGui::End();

        // end ImGui Content
        rlImGuiEnd();

        EndDrawing();
        //----------------------------------------------------------------------------------

        // 推进物理模拟
        scene->simulate(1.0f / 30.0f);
        scene->fetchResults(true);
    }
    rlImGuiShutdown();

    // 释放PhysX资源
    dynamic->release();
    shape->release();
    material->release();
    scene->release();
    dispatcher->release();
    physics->release();
    foundation->release();

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
