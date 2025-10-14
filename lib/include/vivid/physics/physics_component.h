#pragma once

#include <flecs.h>

namespace VIVID {
namespace PHYSICS {
struct Position {
  float x;
  float y;
  float z;
};
struct Velocity {
  float x;
  float y;
  float z;
};

struct PhysicsComponents {
  PhysicsComponents(flecs::world& world) {
    // Register module
    world.module<PhysicsComponents>();

    // Register components
    world.component<Position>();
    world.component<Velocity>();
  };
};

}  // namespace PHYSICS
}  // namespace VIVID
