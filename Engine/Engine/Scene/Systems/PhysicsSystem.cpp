#include "entt/entity/fwd.hpp"
#include <Scene/Systems/PhysicsSystem.h>
#include <Scene/Entity.h>
#include <Engine/Core/Log.h>

#include <algorithm>

namespace SIMPEngine{
  void PhysicsSystem::startFrame(float dt)
  {
        (void)dt;

        for(int i = 0; i < substep; i++)
        {
          physicsWorld.startFrame();
        }
  }

  void PhysicsSystem::UpdateTransform(entt::registry & registry){
        auto view2 = registry.view<TransformComponent, PhysicsComponent>();
        for (auto entity : view2)
        {
            auto &transform = view2.get<TransformComponent>(entity);
            auto &phys = view2.get<PhysicsComponent>(entity);

            if (!phys.body)
                continue;
            transform.position = {phys.body->position.x, phys.body->position.y};
            transform.rotation = glm::degrees(phys.body->orientation);
        }
  }

  void PhysicsSystem::Update(entt::registry & m_Registry, float dt){
      m_Contacts.clear();
      float subdt = dt / static_cast<float>(substep);
      for(int i = 0; i < substep; i++)
      {
        physicsRegistry.updateForces(subdt);
        physicsWorld.step(subdt, 1);
      }

      for (auto &ev : physicsWorld.GetCollisionEvents())
        {
            entt::entity ea = static_cast<entt::entity>(ev.a->entityID);
            entt::entity eb = static_cast<entt::entity>(ev.b->entityID);

            if (!m_Registry.valid(ea) || !m_Registry.valid(eb))
                continue;

            CORE_TRACE("Collision detected between entities {} and {}", static_cast<uint32_t>(ea), static_cast<uint32_t>(eb));

            const bool known = std::any_of(m_Contacts.begin(), m_Contacts.end(), [ea, eb](const ContactPair &pair)
                                           { return (pair.a == ea && pair.b == eb) || (pair.a == eb && pair.b == ea); });
            if (!known)
                m_Contacts.push_back({ea, eb});
        }
  }

  void PhysicsSystem::DestroyBodies(){
    bodies.clear();
  }

  void PhysicsSystem::DestroyWorld(){
    physicsWorld.clear();
  }
}
