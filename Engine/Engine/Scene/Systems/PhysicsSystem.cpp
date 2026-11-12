#include <Engine/Scene/Systems/PhysicsSystem.h>

#include <Engine/Core/Log.h>

#include <algorithm>
#include <cmath>

namespace SIMPEngine
{
    namespace
    {
        constexpr float MaxStep = 1.0f / 30.0f;
        constexpr float PositionEpsilon = 1e-3f;
        constexpr float AngleEpsilon = 1e-4f;
        constexpr int ThreadedBodyCount = 256;

        glm::mat4 ChainMatrix(entt::registry &registry, entt::entity entity)
        {
            glm::mat4 world = registry.get<TransformComponent>(entity).GetLocalTransform();
            entt::entity parent = entt::null;
            if (const auto *hierarchy = registry.try_get<HierarchyComponent>(entity))
                parent = hierarchy->parent;
            for (int depth = 0; depth < 64 && parent != entt::null && registry.valid(parent); ++depth)
            {
                const auto *transform = registry.try_get<TransformComponent>(parent);
                if (!transform)
                    break;
                world = transform->GetLocalTransform() * world;
                const auto *hierarchy = registry.try_get<HierarchyComponent>(parent);
                parent = hierarchy ? hierarchy->parent : entt::null;
            }
            return world;
        }

        glm::mat4 ParentMatrix(entt::registry &registry, entt::entity entity, bool &hasParent)
        {
            hasParent = false;
            const auto *hierarchy = registry.try_get<HierarchyComponent>(entity);
            if (!hierarchy || hierarchy->parent == entt::null || !registry.valid(hierarchy->parent) || !registry.all_of<TransformComponent>(hierarchy->parent))
                return glm::mat4(1.0f);
            hasParent = true;
            return ChainMatrix(registry, hierarchy->parent);
        }

        void WorldPose(entt::registry &registry, entt::entity entity, glm::vec2 &position, float &angle)
        {
            const glm::mat4 world = ChainMatrix(registry, entity);
            position = {world[3][0], world[3][1]};
            angle = std::atan2(world[0][1], world[0][0]);
        }

        float AngleDifference(float a, float b)
        {
            return std::remainder(a - b, 6.28318531f);
        }

        uint64_t PairKey(entt::entity a, entt::entity b)
        {
            const uint32_t x = static_cast<uint32_t>(a);
            const uint32_t y = static_cast<uint32_t>(b);
            return x < y ? (static_cast<uint64_t>(x) << 32) | y : (static_cast<uint64_t>(y) << 32) | x;
        }

        entt::entity EntityOf(const AccelEngine::RigidBody *body)
        {
            return static_cast<entt::entity>(body->entityID);
        }
    }

    void PhysicsSystem::Register(AccelEngine::RigidBody *body)
    {
        if (m_Registered.insert(body).second)
            m_World.addBody(body);
    }

    void PhysicsSystem::Unregister(AccelEngine::RigidBody *body)
    {
        if (m_Registered.erase(body))
            m_World.removeBody(body);
    }

    void PhysicsSystem::AddBody(entt::registry &registry, entt::entity entity, std::unique_ptr<AccelEngine::RigidBody> body)
    {
        if (!body || !registry.valid(entity))
            return;
        PhysicsComponent &component = registry.get_or_emplace<PhysicsComponent>(entity);
        if (component.body)
            RemoveBody(registry, entity);
        component.body = std::move(body);
        component.body->entityID = static_cast<uint32_t>(entity);
        component.synced = false;
        Register(component.body.get());
        SyncBodyToTransform(registry, entity);
    }

    void PhysicsSystem::RemoveBody(entt::registry &registry, entt::entity entity)
    {
        PhysicsComponent *component = registry.try_get<PhysicsComponent>(entity);
        if (!component || !component->body)
            return;
        Unregister(component->body.get());
        component->body.reset();
        component->synced = false;
    }

    void PhysicsSystem::OnBodyDestroyed(AccelEngine::RigidBody *body)
    {
        if (body)
            Unregister(body);
    }

    void PhysicsSystem::RegisterMissing(entt::registry &registry)
    {
        for (entt::entity entity : registry.view<PhysicsComponent>())
        {
            PhysicsComponent &component = registry.get<PhysicsComponent>(entity);
            if (!component.body)
                continue;
            component.body->entityID = static_cast<uint32_t>(entity);
            if (!m_Registered.count(component.body.get()))
            {
                component.synced = false;
                Register(component.body.get());
            }
        }
    }

    void PhysicsSystem::MarkAllUnsynced(entt::registry &registry)
    {
        for (entt::entity entity : registry.view<PhysicsComponent>())
            registry.get<PhysicsComponent>(entity).synced = false;
    }

    void PhysicsSystem::SyncBodyToTransform(entt::registry &registry, entt::entity entity)
    {
        PhysicsComponent *component = registry.try_get<PhysicsComponent>(entity);
        if (!component || !component->body || !registry.all_of<TransformComponent>(entity))
            return;
        glm::vec2 position;
        float angle = 0.0f;
        WorldPose(registry, entity, position, angle);
        component->body->setPose({position.x, position.y}, angle);
        component->lastPosition = position;
        component->lastAngle = angle;
        component->synced = true;
    }

    void PhysicsSystem::PushTransforms(entt::registry &registry)
    {
        auto view = registry.view<PhysicsComponent, TransformComponent>();
        for (entt::entity entity : view)
        {
            PhysicsComponent &component = view.get<PhysicsComponent>(entity);
            AccelEngine::RigidBody *body = component.body.get();
            if (!body)
                continue;

            glm::vec2 position;
            float angle = 0.0f;
            WorldPose(registry, entity, position, angle);

            if (!component.synced)
            {
                body->position = {position.x, position.y};
                body->orientation = angle;
                body->calculateDerivativeData();
                body->wakeUp();
                component.lastPosition = position;
                component.lastAngle = angle;
                component.synced = true;
                continue;
            }

            const glm::vec2 moved = position - component.lastPosition;
            if (std::abs(moved.x) <= PositionEpsilon && std::abs(moved.y) <= PositionEpsilon && std::abs(AngleDifference(angle, component.lastAngle)) <= AngleEpsilon)
                continue;

            const AccelEngine::Vector2 oldMin = body->worldAABBMin, oldMax = body->worldAABBMax;
            body->setPose({position.x, position.y}, angle);
            if (body->isStatic())
            {
                std::vector<AccelEngine::RigidBody *> neighbours;
                const AccelEngine::real margin = m_World.settings.linearSlop * 2.0f + 2.0f;
                const AccelEngine::Vector2 pad(margin, margin);
                m_World.queryAABB(oldMin - pad, oldMax + pad, neighbours);
                m_World.queryAABB(body->worldAABBMin - pad, body->worldAABBMax + pad, neighbours);
                for (AccelEngine::RigidBody *neighbour : neighbours)
                    neighbour->wakeUp();
            }
            component.lastPosition = position;
            component.lastAngle = angle;
        }
    }

    void PhysicsSystem::PullBodies(entt::registry &registry)
    {
        auto view = registry.view<PhysicsComponent, TransformComponent>();
        for (entt::entity entity : view)
        {
            PhysicsComponent &component = view.get<PhysicsComponent>(entity);
            AccelEngine::RigidBody *body = component.body.get();
            if (!body || !component.synced || body->isStatic() || !body->isAwake)
                continue;

            const glm::vec2 worldPosition{body->position.x, body->position.y};
            const float worldAngle = body->orientation;
            TransformComponent &transform = view.get<TransformComponent>(entity);

            bool hasParent = false;
            const glm::mat4 parent = ParentMatrix(registry, entity, hasParent);
            if (!hasParent)
            {
                transform.position = worldPosition;
                transform.rotation = glm::degrees(worldAngle);
            }
            else
            {
                const glm::vec4 local = glm::inverse(parent) * glm::vec4(worldPosition, 0.0f, 1.0f);
                transform.position = {local.x, local.y};
                transform.rotation = glm::degrees(worldAngle - std::atan2(parent[0][1], parent[0][0]));
            }
            transform.dirty = true;
            component.lastPosition = worldPosition;
            component.lastAngle = worldAngle;
        }
    }

    void PhysicsSystem::CollectEvents(entt::registry &registry)
    {
        std::unordered_set<uint64_t> solidSeen;
        std::unordered_set<uint64_t> sensorNow;

        for (const AccelEngine::CollisionEvent &event : m_World.GetCollisionEvents())
        {
            if (!event.a || !event.b)
                continue;
            const entt::entity a = EntityOf(event.a);
            const entt::entity b = EntityOf(event.b);
            if (!registry.valid(a) || !registry.valid(b))
                continue;

            const uint64_t key = PairKey(a, b);
            if (event.sensor)
                sensorNow.insert(key);
            else if (solidSeen.insert(key).second)
                m_Contacts.push_back({a, b});
        }

        auto asleepPair = [&](uint64_t key)
        {
            const entt::entity a = static_cast<entt::entity>(static_cast<uint32_t>(key >> 32));
            const entt::entity b = static_cast<entt::entity>(static_cast<uint32_t>(key & 0xFFFFFFFFu));
            if (!registry.valid(a) || !registry.valid(b))
                return false;
            const PhysicsComponent *pa = registry.try_get<PhysicsComponent>(a);
            const PhysicsComponent *pb = registry.try_get<PhysicsComponent>(b);
            return pa && pb && pa->body && pb->body && (!pa->body->isAwake || !pb->body->isAwake);
        };

        for (uint64_t key : sensorNow)
        {
            const entt::entity a = static_cast<entt::entity>(static_cast<uint32_t>(key >> 32));
            const entt::entity b = static_cast<entt::entity>(static_cast<uint32_t>(key & 0xFFFFFFFFu));
            m_SensorEvents.push_back({a, b, m_SensorPairs.count(key) ? SensorEventType::Stay : SensorEventType::Enter});
        }

        std::unordered_set<uint64_t> next = sensorNow;
        for (uint64_t key : m_SensorPairs)
        {
            if (sensorNow.count(key))
                continue;
            const entt::entity a = static_cast<entt::entity>(static_cast<uint32_t>(key >> 32));
            const entt::entity b = static_cast<entt::entity>(static_cast<uint32_t>(key & 0xFFFFFFFFu));
            if (!registry.valid(a) || !registry.valid(b))
                continue;
            if (asleepPair(key))
            {
                next.insert(key);
                continue;
            }
            m_SensorEvents.push_back({a, b, SensorEventType::Exit});
        }
        m_SensorPairs = std::move(next);
    }

    void PhysicsSystem::Update(entt::registry &registry, float dt)
    {
        m_Contacts.clear();
        m_SensorEvents.clear();

        RegisterMissing(registry);
        PushTransforms(registry);

        const int bodyCount = m_World.getBodyCount();
        if (bodyCount == 0)
        {
            m_SensorPairs.clear();
            return;
        }
        if (!(dt > 0.0f) || !std::isfinite(dt))
            return;

        const int wanted = bodyCount >= ThreadedBodyCount ? -1 : 0;
        if (m_World.settings.workerThreads != wanted)
            m_World.settings.workerThreads = wanted;

        m_World.step(std::min(dt, MaxStep));

        PullBodies(registry);
        CollectEvents(registry);

        const int disabled = m_World.getDisabledBodyCount();
        if (disabled > m_ReportedDisabled)
            CORE_WARN("Physics: {} body(ies) reached an invalid state (NaN/inf) and were switched off", disabled - m_ReportedDisabled);
        m_ReportedDisabled = disabled;
    }

    void PhysicsSystem::Clear()
    {
        m_World.clear();
        m_Registered.clear();
        m_SensorPairs.clear();
        m_Contacts.clear();
        m_SensorEvents.clear();
        m_ReportedDisabled = 0;
    }

    void PhysicsSystem::ResetSettings()
    {
        m_World.settings = AccelEngine::PhysicsSettings();
        m_World.gravity = AccelEngine::Vector2(0.0f, -980.0f);
    }
}
