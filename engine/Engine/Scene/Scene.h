#pragma once

#include <entt/entt.hpp>
#include <string>

namespace SIMPEngine
{
    class Entity;

    class Scene
    {
    public:
        Scene(const std::string &name = "untitled");
        ~Scene() = default;

        Entity CreateEntity(const std::string &name = "");
        void DestroyEntity(Entity entity);

        void OnUpdate(float dt);
        void OnRender();
        void OnEvent();

        entt::registry &GetRegistry() { return m_Registry; }
        const std::string &GetName() const { return m_Name; }

    private:
        std::string m_Name;
        entt::registry m_Registry;
    };
}