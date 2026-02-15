#pragma once 
#include "AccelEngine/ForceRegistry.h"
#include "entt/entity/fwd.hpp"
#include <AccelEngine/body.h>
#include <AccelEngine/world.h>

namespace SIMPEngine {
class PhysicsSystem {
public:
  void Update(entt::registry & registry, float deltatime);
  void startFrame(float dt);
  void UpdateTransform(entt::registry  & registry);
  void SetSubstep(int substep) {this->substep = substep;}
  void DestroyBodies();
  void DestroyWorld();
  AccelEngine::World  & GetWorld(){
    return physicsWorld;
  }

  std::vector<AccelEngine::RigidBody *> bodies;
private:
  AccelEngine::World physicsWorld;
  AccelEngine::ForceRegistry physicsRegistry;

  int substep;
}; 
} // namespace SIMPEngine
