// PhysX + Raylib 3D 多球体坠落演示
#include "raylib.h"
#include "raymath.h"
#include "PxPhysicsAPI.h"
#include <vector>
#include <limits>

int main()
{
    // 初始化PhysX
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

    // 创建静态地面
    physx::PxMaterial *material = physics->createMaterial(0.5f, 0.5f, 0.6f);
    physx::PxRigidStatic *ground = physics->createRigidStatic(physx::PxTransform(physx::PxVec3(0.0f, -0.5f, 0.0f)));
    physx::PxShape *planeShape = physics->createShape(physx::PxBoxGeometry(20.0f, 0.5f, 20.0f), *material); // 物理地面40x40
    ground->attachShape(*planeShape);
    scene->addActor(*ground);

    // 创建四周围栏
    float fenceHeight = 10.0f;
    float fenceThickness = 1.0f;
    float fenceLength = 40.0f;
    std::vector<physx::PxRigidStatic *> fences;
    // +X
    physx::PxRigidStatic *fencePX = physics->createRigidStatic(physx::PxTransform(physx::PxVec3(20.0f + fenceThickness / 2, fenceHeight / 2, 0.0f)));
    fencePX->attachShape(*physics->createShape(physx::PxBoxGeometry(fenceThickness / 2, fenceHeight / 2, fenceLength / 2), *material));
    scene->addActor(*fencePX);
    fences.push_back(fencePX);
    // -X
    physx::PxRigidStatic *fenceNX = physics->createRigidStatic(physx::PxTransform(physx::PxVec3(-20.0f - fenceThickness / 2, fenceHeight / 2, 0.0f)));
    fenceNX->attachShape(*physics->createShape(physx::PxBoxGeometry(fenceThickness / 2, fenceHeight / 2, fenceLength / 2), *material));
    scene->addActor(*fenceNX);
    fences.push_back(fenceNX);
    // +Z
    physx::PxRigidStatic *fencePZ = physics->createRigidStatic(physx::PxTransform(physx::PxVec3(0.0f, fenceHeight / 2, 20.0f + fenceThickness / 2)));
    fencePZ->attachShape(*physics->createShape(physx::PxBoxGeometry(fenceLength / 2, fenceHeight / 2, fenceThickness / 2), *material));
    scene->addActor(*fencePZ);
    fences.push_back(fencePZ);
    // -Z
    physx::PxRigidStatic *fenceNZ = physics->createRigidStatic(physx::PxTransform(physx::PxVec3(0.0f, fenceHeight / 2, -20.0f - fenceThickness / 2)));
    fenceNZ->attachShape(*physics->createShape(physx::PxBoxGeometry(fenceLength / 2, fenceHeight / 2, fenceThickness / 2), *material));
    scene->addActor(*fenceNZ);
    fences.push_back(fenceNZ);

    // 创建10个动态球体
    float sphereRadius = 1.0f;
    int ballCount = 10;
    std::vector<physx::PxRigidDynamic *> balls;
    std::vector<Color> ballColors;
    float spacing = 3.0f; // 增大间距，避免初始重叠
    float startX = -((ballCount - 1) * spacing) / 2.0f;
    for (int i = 0; i < ballCount; ++i)
    {
        float x = startX + i * spacing;
        physx::PxShape *sphereShape = physics->createShape(physx::PxSphereGeometry(sphereRadius), *material);
        physx::PxTransform startTransform(physx::PxVec3(x, 10.0f, 0.0f));
        physx::PxRigidDynamic *dynamic = physics->createRigidDynamic(startTransform);
        dynamic->attachShape(*sphereShape);
        physx::PxRigidBodyExt::updateMassAndInertia(*dynamic, 1.0f);
        scene->addActor(*dynamic);
        balls.push_back(dynamic);
        ballColors.push_back(Color{(unsigned char)(100 + i * 15), (unsigned char)(50 + i * 20), (unsigned char)(200 - i * 10), 255});
    }

    // Raylib窗口和摄像机
    int screenWidth = 1280;
    int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "PhysX + Raylib 3D 多球体坠落演示");
    SetTargetFPS(120);

    Camera3D camera = {0};
    camera.position = Vector3{10.0f, 10.0f, 20.0f};
    camera.target = Vector3{0.0f, 2.0f, 0.0f};
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    int draggingBall = -1; // 当前被拖动的球体索引
    bool draggingXZ = false;
    bool draggingY = false;
    float dragYStart = 0.0f;
    float dragYOrigin = 0.0f;

    while (!WindowShouldClose())
    {
        Vector3 dragTarget = {0};

        // 鼠标左键单击空白处添加新球体
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            Ray ray = GetMouseRay(GetMousePosition(), camera);
            // 检查是否点中已有球体
            int hitBall = -1;
            for (int i = 0; i < (int)balls.size(); ++i)
            {
                physx::PxTransform pose = balls[i]->getGlobalPose();
                Vector3 pos = {pose.p.x, pose.p.y, pose.p.z};
                if (GetRayCollisionSphere(ray, pos, sphereRadius).hit)
                {
                    hitBall = i;
                    break;
                }
            }
            // 没有点中球体，添加新球
            if (hitBall == -1)
            {
                RayCollision col = GetRayCollisionQuad(ray, Vector3{-20, 0, -20}, Vector3{20, 0, -20}, Vector3{20, 0, 20}, Vector3{-20, 0, 20});
                if (col.hit)
                {
                    Vector3 pos = col.point;
                    float y = 10.0f; // 新球体初始高度
                    physx::PxShape *sphereShape = physics->createShape(physx::PxSphereGeometry(sphereRadius), *material);
                    physx::PxTransform startTransform(physx::PxVec3(pos.x, y, pos.z));
                    physx::PxRigidDynamic *dynamic = physics->createRigidDynamic(startTransform);
                    dynamic->attachShape(*sphereShape);
                    physx::PxRigidBodyExt::updateMassAndInertia(*dynamic, 1.0f);
                    scene->addActor(*dynamic);
                    balls.push_back(dynamic);
                    int i = (int)balls.size() - 1;
                    ballColors.push_back(Color{(unsigned char)(100 + i * 15), (unsigned char)(50 + i * 20), (unsigned char)(200 - i * 10), 255});
                }
            }
        }

        // 鼠标检测最近球体
        int hoveredBall = -1;
        float minDist = std::numeric_limits<float>::max();
        Ray ray = GetMouseRay(GetMousePosition(), camera);
        for (int i = 0; i < (int)balls.size(); ++i)
        {
            physx::PxTransform pose = balls[i]->getGlobalPose();
            Vector3 pos = {pose.p.x, pose.p.y, pose.p.z};
            RayCollision col = GetRayCollisionSphere(ray, pos, sphereRadius);
            if (col.hit && col.distance < minDist)
            {
                minDist = col.distance;
                hoveredBall = i;
            }
        }

        // 左键XZ平面拖动
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoveredBall != -1)
        {
            draggingBall = hoveredBall;
            draggingXZ = true;
            balls[draggingBall]->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && draggingXZ && draggingBall != -1)
        {
            draggingXZ = false;
            balls[draggingBall]->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, false);
            balls[draggingBall]->setLinearVelocity(physx::PxVec3(0, 0, 0));
            balls[draggingBall]->setAngularVelocity(physx::PxVec3(0, 0, 0));
            draggingBall = -1;
        }
        if (draggingXZ && draggingBall != -1)
        {
            physx::PxTransform pose = balls[draggingBall]->getGlobalPose();
            float planeY = pose.p.y;
            Vector3 planeNormal = {0, 1, 0};
            float denom = Vector3DotProduct(planeNormal, ray.direction);
            if (fabsf(denom) > 1e-6f)
            {
                float t = (planeY - Vector3DotProduct(planeNormal, ray.position)) / denom;
                if (t > 0)
                {
                    dragTarget = Vector3Add(ray.position, Vector3Scale(ray.direction, t));
                    dragTarget.y = planeY;
                    balls[draggingBall]->setKinematicTarget(physx::PxTransform(physx::PxVec3(dragTarget.x, dragTarget.y, dragTarget.z)));
                }
            }
        }

        // 右键Y轴拖动
        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && hoveredBall != -1)
        {
            draggingBall = hoveredBall;
            draggingY = true;
            dragYStart = GetMousePosition().y;
            dragYOrigin = balls[draggingBall]->getGlobalPose().p.y;
            balls[draggingBall]->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
        }
        if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON) && draggingY && draggingBall != -1)
        {
            draggingY = false;
            balls[draggingBall]->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, false);
            balls[draggingBall]->setLinearVelocity(physx::PxVec3(0, 0, 0));
            balls[draggingBall]->setAngularVelocity(physx::PxVec3(0, 0, 0));
            draggingBall = -1;
        }
        if (draggingY && draggingBall != -1)
        {
            float mouseDelta = GetMousePosition().y - dragYStart;
            float sensitivity = 0.03f;
            float newY = dragYOrigin - mouseDelta * sensitivity;
            if (newY < sphereRadius)
                newY = sphereRadius;
            physx::PxVec3 cur = balls[draggingBall]->getGlobalPose().p;
            balls[draggingBall]->setKinematicTarget(physx::PxTransform(physx::PxVec3(cur.x, newY, cur.z)));
        }

        // 推进物理模拟
        scene->simulate(1.0f / 120.0f);
        scene->fetchResults(true);

        // 绘制
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        DrawCube(Vector3{0, -0.5f, 0}, 40, 1, 40, LIGHTGRAY);

        // 绘制围栏
        DrawCube(Vector3{20.5f, 5.0f, 0}, 1, 10, 40, GRAY);
        DrawCube(Vector3{-20.5f, 5.0f, 0}, 1, 10, 40, GRAY);
        DrawCube(Vector3{0, 5.0f, 20.5f}, 40, 10, 1, GRAY);
        DrawCube(Vector3{0, 5.0f, -20.5f}, 40, 10, 1, GRAY);

        // 绘制所有球体
        for (int i = 0; i < (int)balls.size(); ++i)
        {
            physx::PxTransform pose = balls[i]->getGlobalPose();
            Vector3 pos = {pose.p.x, pose.p.y, pose.p.z};
            Color color = (i == draggingBall) ? ORANGE : ballColors[i];
            DrawSphere(pos, sphereRadius, color);

            // 拖动时显示gizmo
            if (i == draggingBall && (draggingXZ || draggingY))
            {
                float axisLen = 2.0f;
                DrawLine3D(pos, Vector3Add(pos, Vector3{axisLen, 0, 0}), RED);
                DrawLine3D(pos, Vector3Add(pos, Vector3{0, axisLen, 0}), GREEN);
                DrawLine3D(pos, Vector3Add(pos, Vector3{0, 0, axisLen}), BLUE);
            }
        }

        DrawGrid(20, 1.0f);

        EndMode3D();

        DrawText("PhysX + Raylib 3D 多球体坠落演示", 10, 10, 20, DARKGRAY);
        if (draggingBall != -1)
        {
            DrawText(TextFormat("拖动球体: %d", draggingBall), 10, 40, 20, ORANGE);
            if (draggingXZ)
                DrawText("XZ平面拖动", 10, 70, 20, ORANGE);
            if (draggingY)
                DrawText("Y轴拖动", 10, 100, 20, ORANGE);
        }

        EndDrawing();
    }

    // 释放资源
    for (int i = 0; i < (int)balls.size(); ++i)
    {
        balls[i]->release();
        // PxShape由PhysX自动管理，无需手动释放
    }
    ground->release();
    planeShape->release();
    // 释放围栏
    for (auto *fence : fences)
        fence->release();
    material->release();
    scene->release();
    dispatcher->release();
    physics->release();
    foundation->release();

    CloseWindow();
    return 0;
}
