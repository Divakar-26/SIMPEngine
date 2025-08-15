#pragma once

#include <entt/entt.hpp>
#include "Components/TransformComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/VelocityComponent.h"
#include "Components/CollisionComponent.h"
#include "Components/TagComponent.h"

#include <string>

namespace SIMPEngine
{
    class Entity;

    class Scene
    {
    public:
        Scene(const std::string &name = "untitled");
        ~Scene() = default;

        void DestroyEntity(Entity entity);

        void OnUpdate(float dt);
        void OnRender();
        void OnEvent();

        Entity CreatePlayer();
        Entity CreateEntity(const std::string &name = "");
        Entity GetEntityByName(const std::string &name);

        entt::registry &GetRegistry()
        {
            return m_Registry;
        }
        const std::string &GetName() const { return m_Name; }

    private:
        std::string m_Name;
        entt::registry m_Registry;
    };
}