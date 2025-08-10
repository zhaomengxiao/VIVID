#pragma once
#include "entt/entt.hpp"
#include "physics_component.h"
#include "PxPhysicsAPI.h"

// 物理系统：推进物理世界，并同步状态回ECS
void physics_system(entt::registry &registry, physx::PxScene &scene, float deltaTime)
{
    // 1. 推进物理模拟
    scene.simulate(deltaTime);
    scene.fetchResults(true);

    // 2. 将物理世界的状态同步回ECS组件
    //    这样渲染系统就能画出正确的位置
    auto view = registry.view<TransformComponent, const RigidBodyComponent>();
    for (auto [entity, transform, rigidbody] : view.each())
    {
        if (rigidbody.actor && rigidbody.actor->is<physx::PxRigidDynamic>())
        {
            physx::PxTransform physx_transform = rigidbody.actor->getGlobalPose();

            // 更新我们自己的TransformComponent
            transform.position.x = physx_transform.p.x;
            transform.position.y = physx_transform.p.y;
            transform.position.z = physx_transform.p.z;

            // 如果需要，同样更新旋转 (Quaternion)
            // transform.rotation = ...
        }
    }
}
