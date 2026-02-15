#pragma once

#include <entt/entt.hpp>

namespace SIMPEngine{
    class RenderSystem{

        public:
            void RenderQuads(entt::registry & m_Registry);
            void RenderSprites(entt::registry & m_Registry); 
    };
}