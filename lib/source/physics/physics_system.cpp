#include "vivid/physics/physics_system.h"

#include <iostream>

VIVID::PHYSICS::PhysicsSystems::PhysicsSystems(flecs::world& world) {
  // Register module
  world.module<PhysicsSystems>();
  // import components module
  world.import <PhysicsComponents>();

  // Register systems
  world.system<Position, Velocity>("MoveSystem").each(moveImpl);
}

// 静态成员函数实现
void VIVID::PHYSICS::PhysicsSystems::moveImpl(flecs::entity e, Position& pos, Velocity& vel) {
  pos.x += vel.x;
  pos.y += vel.y;
  pos.z += vel.z;

  std::cout << "MoveSystem: " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
}