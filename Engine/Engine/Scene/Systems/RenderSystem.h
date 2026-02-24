#pragma once

#include <entt/entt.hpp>
#include <utility>
#include <vector>

namespace SIMPEngine{
    class RenderSystem{

        public:
            void RenderQuads(entt::registry & m_Registry);
            void RenderSprites(entt::registry & m_Registry); 
        
        private:
            std::vector<std::pair<entt::entity, float>> m_QuadSortBuffer;
            std::vector<std::pair<entt::entity, float>> m_SpriteSortBuffer;
    };
}