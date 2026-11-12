#pragma once

#include <Engine/Scene/Entity.h>
#include <Engine/Scene/ScriptEvent.h>

namespace SIMPEngine
{
    struct ScriptMessage
    {
        ScriptEvent Type = ScriptEvent::FixedUpdate;
        Entity Other;
        float Value = 0.0f;
    };

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

        void SendEvent(const ScriptMessage &message) { OnEvent(message); }

    protected:
        virtual void OnCreate() {}
        virtual void OnDestroy() {}
        virtual void OnUpdate(float) {}
        virtual void OnRender()  {}
        virtual void OnFixedUpdate(float) {}
        virtual void OnTriggerEnter(Entity) {}
        virtual void OnTriggerStay(Entity) {}
        virtual void OnTriggerExit(Entity) {}
        virtual void OnCollision(Entity) {}
        virtual void OnTimer() {}
        virtual void OnDamaged(float, Entity) {}
        virtual void OnDeath() {}
        virtual void OnPathPoint(int) {}
        virtual void OnPathEnd() {}
        virtual void OnLand(float) {}
        virtual void OnJump() {}
        virtual void OnGrab() {}
        virtual void OnDrop() {}
        virtual void OnClick() {}
        virtual void OnHoverEnter() {}
        virtual void OnHoverExit() {}
        virtual void OnEvent(const ScriptMessage &message);

    private:
        Entity m_Entity;

        friend class Scene;
    };
}
