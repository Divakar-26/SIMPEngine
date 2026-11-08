#pragma once
#include "AccelEngine/ForceRegistry.h"
#include "entt/entity/fwd.hpp"
#include <AccelEngine/body.h>
#include <AccelEngine/world.h>
#include <Engine/Scene/Component.h>

namespace SIMPEngine {
class PhysicsSystem {
public:
  struct ContactPair {
    entt::entity a = entt::null;
    entt::entity b = entt::null;
  };

  void Update(entt::registry & registry, float deltatime);
  void startFrame(float dt);
  void UpdateTransform(entt::registry  & registry);
  void SetSubstep(int substep) {this->substep = substep;}
  void DestroyBodies();
  void DestroyWorld();
  const std::vector<ContactPair> &GetContacts() const { return m_Contacts; }
  AccelEngine::World  & GetWorld(){
    return physicsWorld;
  }

  std::vector<AccelEngine::RigidBody *> bodies;
private:
  AccelEngine::World physicsWorld;
  AccelEngine::ForceRegistry physicsRegistry;

  std::vector<ContactPair> m_Contacts;

  int substep = 1;
};
}
