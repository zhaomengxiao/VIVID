#pragma once
#include <flecs.h>

#include "physics_component.h"

namespace VIVID {
namespace PHYSICS {

struct PhysicsSystems {
  PhysicsSystems(flecs::world& world);

private:
  // 静态成员函数声明
  static void moveImpl(flecs::entity e, Position& pos, Velocity& vel);
};

}  // namespace PHYSICS
}  // namespace VIVID
