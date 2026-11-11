#pragma once
#include <Engine/Scene/Component.h>

#include <entt/entt.hpp>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace SIMPEngine
{
    class Scene;
    class ScriptableEntity;

    class SpawnerSystem
    {
    public:
        void Update(Scene &scene, float dt);
        void Clear();

        static entt::entity Clone(Scene &scene, entt::entity source);

    private:
        struct Prototype
        {
            std::string Yaml;
            std::vector<std::pair<size_t, std::function<std::unique_ptr<ScriptableEntity>()>>> Scripts;
        };

        struct Request
        {
            entt::entity Spawner = entt::null;
            int Count = 0;
        };

        const Prototype *PrototypeFor(Scene &scene, entt::entity spawner, const std::string &name, bool hide);

        std::unordered_map<std::string, Prototype> m_Prototypes;
        std::unordered_set<std::string> m_Missing;
        std::vector<Request> m_Requests;
        std::mt19937 m_Random{std::random_device{}()};
    };
}
