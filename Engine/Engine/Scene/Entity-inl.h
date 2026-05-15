#pragma once

#include <entt/entt.hpp>

namespace SIMPEngine
{
    template<typename T, typename... Args>
    T& Entity::AddComponent(Args&&... args)
    {
        return m_Scene->GetRegistry().template emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
    }

    template<typename T>
    T& Entity::GetComponent()
    {
        return m_Scene->GetRegistry().template get<T>(m_EntityHandle);
    }

    template<typename T>
    bool Entity::HasComponent()
    {
        return m_Scene->GetRegistry().template any_of<T>(m_EntityHandle);
    }

    template<typename T>
    void Entity::RemoveComponent()
    {
        m_Scene->GetRegistry().template remove<T>(m_EntityHandle);
    }
}
