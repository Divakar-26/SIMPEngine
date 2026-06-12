#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <string>

// Forward declaration for use in templates
namespace SIMPEngine
{
    class Scene;

    class Entity
    {
    public:
        Entity() = default;
        Entity(entt::entity handle, Scene* scene)
            : m_EntityHandle(handle), m_Scene(scene) {}

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args);

        template<typename T>
        T& GetComponent();

        template<typename T> 
        bool HasComponent();

        template<typename T>
        void RemoveComponent();

        entt::entity GetHandle() const { return m_EntityHandle; }

        operator bool() const { return m_EntityHandle != entt::null; }
        operator entt::entity() const { return m_EntityHandle; }

    private:
        entt::entity m_EntityHandle{ entt::null };
        Scene* m_Scene = nullptr;
    };
}

#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Entity-inl.h>
