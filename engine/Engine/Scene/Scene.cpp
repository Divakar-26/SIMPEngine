#include "Scene.h"
#include "Entity.h"
#include <iostream>

namespace SIMPEngine
{
    Scene::Scene(const std::string& name)
        : m_Name(name)
    {
    }

    Entity Scene::CreateEntity(const std::string& name)
    {
        entt::entity entityHandle = m_Registry.create();
        Entity entity(entityHandle, this);
        // Optional: Add default components like Transform
        return entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        m_Registry.destroy(entity.GetHandle());
    }

    void Scene::OnUpdate(float dt)
    {
        
        //  physics animation ai
    }

    void Scene::OnRender()
    {
        
    }
}
