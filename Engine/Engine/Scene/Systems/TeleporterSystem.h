#pragma once
#include <Engine/Scene/Component.h>

#include <unordered_set>

namespace SIMPEngine
{
    class Scene;

    class TeleporterSystem
    {
    public:
        void Update(Scene &scene);
        void Clear() { m_Blocked.clear(); }

    private:
        std::unordered_set<entt::entity> m_Blocked;
    };
}
