#pragma once
#include <Engine/Scene/Component.h>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include <vector>

namespace SIMPEngine
{
    class Scene;

    class UISystem
    {
    public:
        void Update(Scene &scene, float dt);
        void Render(Scene &scene, bool screenSpace);
        void Clear();

        bool IsMouseOverUI() const { return m_MouseOverUI; }
        entt::entity GetFocused() const { return m_Focus; }
        bool IsFocusVisible() const { return m_FocusVisible; }

        static void Layout(entt::registry &registry, const glm::vec2 &viewport);
        static glm::vec2 ToCanvas(const CanvasComponent &canvas, const glm::vec2 &viewportPoint, const glm::vec2 &viewport);

    private:
        struct Node
        {
            entt::entity Entity = entt::null;
            entt::entity Canvas = entt::null;
        };

        static void Gather(entt::registry &registry, std::vector<Node> &nodes, std::vector<entt::entity> &canvases);
        void DrawNode(entt::registry &registry, entt::entity entity, const glm::vec4 &tint, float &z);
        void Navigate(entt::registry &registry, const std::vector<Node> &nodes, const glm::vec2 &direction);

        bool m_MouseOverUI = false;
        bool m_FocusVisible = false;
        entt::entity m_Focus = entt::null;
        glm::vec2 m_LastMouse{-1.0f, -1.0f};
    };
}
