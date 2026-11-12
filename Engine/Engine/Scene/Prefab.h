#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include <optional>
#include <string>

namespace SIMPEngine
{
    class Scene;

    namespace Prefab
    {
        bool IsPrefabPath(const std::string &path);
        std::optional<std::string> LoadYaml(const std::string &path);
        bool Save(Scene &scene, entt::entity root, const std::string &path);
        entt::entity Instantiate(Scene &scene, const std::string &path, const std::optional<glm::vec2> &position = std::nullopt, entt::entity parent = entt::null);
        entt::entity Refresh(Scene &scene, entt::entity instance);
        void ClearCache();
    }
}
