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

    protected:
        virtual void OnCreate() {}
        virtual void OnDestroy() {}
        virtual void OnUpdate(float dt) {}

    private:
        Entity m_Entity;

        friend class Scene;
    };
}
