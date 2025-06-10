#pragma once
#include "PxPhysicsAPI.h" // PhysX header

// 简单的变换组件（位置）
struct TransformComponent
{
    physx::PxVec3 position{0.0f, 0.0f, 0.0f};
    // 可扩展旋转等
};

// 代表一个刚体物理对象
struct RigidBodyComponent
{
    physx::PxRigidActor *actor = nullptr;
    // 这里可以添加更多属性，如质量、阻尼等
};
