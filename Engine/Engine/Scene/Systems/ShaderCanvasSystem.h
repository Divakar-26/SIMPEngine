#pragma once
#include <Engine/Scene/Component.h>

#include <entt/entt.hpp>

#include <memory>
#include <unordered_map>
#include <vector>

namespace SIMPEngine
{
    class Scene;
    class Framebuffer;

    class ShaderCanvasSystem
    {
    public:
        ~ShaderCanvasSystem();

        void Prepare(Scene &scene, bool apply);
        void RenderGroups(Scene &scene);
        void RenderPost(Scene &scene);
        void RenderOne(Scene &scene, ShaderCanvasComponent &canvas, entt::entity entity = entt::null);
        void Clear();

        static void SyncUniforms(ShaderCanvasComponent &canvas);
        static ShaderUniformValue *FindUniform(ShaderCanvasComponent &canvas, const std::string &name);
        static void ReleaseShared();

    private:
        std::vector<entt::entity> Ordered(entt::registry &registry, bool groups) const;
        bool AreaPixels(Scene &scene, entt::entity entity, const ShaderCanvasComponent &canvas, const glm::vec2 &viewport, int rect[4]) const;
        void Composite(Scene &scene, ShaderCanvasComponent &canvas, unsigned int sourceTexture, bool blend, int width, int height, const int *area = nullptr);

        std::unordered_map<entt::entity, std::vector<entt::entity>> m_Members;
        std::unordered_map<entt::entity, std::unique_ptr<Framebuffer>> m_Groups;
        std::unordered_map<const void *, std::unique_ptr<Framebuffer>> m_Copies;
    };
}
