#include "entt/entity/fwd.hpp"
#include <Scene/Systems/PhysicsSystem.h>
#include <Scene/Entity.h>
#include <Engine/Core/Log.h>

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
            transform.rotation = phys.body->orientation * (180.0f / 3.14159f);
        }
  }

  void PhysicsSystem::Update(entt::registry & m_Registry, float dt){
      float subdt = dt / substep; 
      for(int i = 0; i < substep; i++)
      {
        physicsRegistry.updateForces(subdt);
        physicsWorld.step(subdt, 1);
      }

      // collision checking
      for (auto &ev : physicsWorld.GetCollisionEvents())
        {
            entt::entity ea = (entt::entity)ev.a->entityID;
            entt::entity eb = (entt::entity)ev.b->entityID;

            if (!m_Registry.valid(ea) || !m_Registry.valid(eb))
                continue;

            // Entity A{ea, this};
            // Entity B{eb, this};

            CORE_TRACE("Collision detected between entities {} and {}", static_cast<uint32_t>(ea), static_cast<uint32_t>(eb));

        }

     
  }


  void PhysicsSystem::DestroyBodies(){
    for(auto body : bodies)
    {
      delete body;
    }
    bodies.clear();
  }

  void PhysicsSystem::DestroyWorld(){
    physicsWorld.clear();     
  }

} 
