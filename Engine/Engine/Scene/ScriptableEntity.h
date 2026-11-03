#pragma once

#include <Engine/Scene/Entity.h>

namespace SIMPEngine
{
    class ScriptableEntity
    {
    public:
        virtual ~ScriptableEntity() = default;

        template<typename T>
        T& GetComponent()
        {
            return m_Entity.GetComponent<T>();
        }

        Entity GetEntity() const { return m_Entity; }

        glm::vec2 GetMouseWorldPosition() const;
        bool IsMouseInViewport() const;

    protected:
        virtual void OnCreate() {}
        virtual void OnDestroy() {}
        virtual void OnUpdate(float dt) {}
        virtual void OnRender()  {}

    private:
        Entity m_Entity;

        friend class Scene;
    };
}
