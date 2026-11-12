#pragma once
#include <AccelEngine/body.h>
#include <AccelEngine/world.h>
#include <Engine/Scene/Component.h>

#include <entt/entt.hpp>

#include <memory>
#include <unordered_set>
#include <vector>

namespace SIMPEngine
{
    class PhysicsSystem
    {
    public:
        enum class SensorEventType
        {
            Enter,
            Stay,
            Exit
        };

        struct ContactPair
        {
            entt::entity a = entt::null;
            entt::entity b = entt::null;
        };

        struct SensorEvent
        {
            entt::entity a = entt::null;
            entt::entity b = entt::null;
            SensorEventType Type = SensorEventType::Enter;
        };

        void Update(entt::registry &registry, float dt);

        void AddBody(entt::registry &registry, entt::entity entity, std::unique_ptr<AccelEngine::RigidBody> body);
        void RemoveBody(entt::registry &registry, entt::entity entity);
        void OnBodyDestroyed(AccelEngine::RigidBody *body);
        void RegisterMissing(entt::registry &registry);
        void MarkAllUnsynced(entt::registry &registry);

        static void SyncBodyToTransform(entt::registry &registry, entt::entity entity);

        void Clear();
        void ResetSettings();

        const std::vector<ContactPair> &GetContacts() const { return m_Contacts; }
        const std::vector<SensorEvent> &GetSensorEvents() const { return m_SensorEvents; }
        AccelEngine::World &GetWorld() { return m_World; }
        const AccelEngine::World &GetWorld() const { return m_World; }

    private:
        void Register(AccelEngine::RigidBody *body);
        void Unregister(AccelEngine::RigidBody *body);
        void PushTransforms(entt::registry &registry);
        void PullBodies(entt::registry &registry);
        void CollectEvents(entt::registry &registry);

        AccelEngine::World m_World;
        std::unordered_set<AccelEngine::RigidBody *> m_Registered;
        std::unordered_set<uint64_t> m_SensorPairs;
        std::vector<ContactPair> m_Contacts;
        std::vector<SensorEvent> m_SensorEvents;
        int m_ReportedDisabled = 0;
    };
}
