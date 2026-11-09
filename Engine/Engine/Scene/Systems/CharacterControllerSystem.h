#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace SIMPEngine
{
    class Scene;

    class CharacterControllerSystem
    {
    public:
        void Update(Scene &scene, float dt);

    private:
        struct Solid
        {
            glm::vec2 Min;
            glm::vec2 Max;
            const std::string *Tag = nullptr;
        };

        std::vector<Solid> m_Solids;
        std::vector<entt::entity> m_Order;
    };
}
