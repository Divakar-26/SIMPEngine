#include "ShaderCanvasSystem.h"
#include "RenderSystem.h"

#include <Engine/Input/Input.h>
#include <Engine/Math/EntityUtility.h>
#include <Engine/Rendering/Framebuffer.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Rendering/ShaderEffect.h>
#include <Engine/Scene/Scene.h>

#include <Engine/Rendering/GL.h>

#include <algorithm>
#include <cmath>

namespace SIMPEngine
{
    namespace
    {
        unsigned int s_Vao = 0;

        void EnsureVao()
        {
            if (!s_Vao)
                glGenVertexArrays(1, &s_Vao);
        }

        void CollectDescendants(entt::registry &registry, entt::entity entity, std::vector<entt::entity> &out, int depth = 0)
        {
            if (depth > 64)
                return;
            const auto *hierarchy = registry.try_get<HierarchyComponent>(entity);
            if (!hierarchy)
                return;
            for (entt::entity child : hierarchy->children)
            {
                if (!registry.valid(child))
                    continue;
                out.push_back(child);
                CollectDescendants(registry, child, out, depth + 1);
            }
        }
    }

    ShaderCanvasSystem::~ShaderCanvasSystem() = default;

    void ShaderCanvasSystem::ReleaseShared()
    {
        if (s_Vao)
        {
            glDeleteVertexArrays(1, &s_Vao);
            s_Vao = 0;
        }
        ShaderEffect::ReleaseAll();
    }

    void ShaderCanvasSystem::Clear()
    {
        m_Members.clear();
        m_Groups.clear();
        m_Copies.clear();
    }

    ShaderUniformValue *ShaderCanvasSystem::FindUniform(ShaderCanvasComponent &canvas, const std::string &name)
    {
        for (ShaderUniformValue &uniform : canvas.uniforms)
            if (uniform.name == name)
                return &uniform;
        return nullptr;
    }

    void ShaderCanvasSystem::SyncUniforms(ShaderCanvasComponent &canvas)
    {
        if (!canvas.effect || !canvas.effect->IsValid())
            return;

        std::vector<ShaderUniformValue> synced;
        for (const ShaderUniformDecl &decl : canvas.effect->GetDeclared())
        {
            ShaderUniformValue value;
            if (ShaderUniformValue *existing = FindUniform(canvas, decl.Name); existing && existing->type == static_cast<int>(decl.Type))
                value = *existing;
            else
            {
                value.name = decl.Name;
                value.type = static_cast<int>(decl.Type);
                for (int i = 0; i < 4; ++i)
                    value.value[i] = decl.Default[i];
            }
            value.min = decl.Min;
            value.max = decl.Max;
            synced.push_back(std::move(value));
        }
        canvas.uniforms = std::move(synced);
    }

    void ShaderCanvasSystem::Prepare(Scene &scene, bool apply)
    {
        entt::registry &registry = scene.GetRegistry();
        registry.clear<ShaderGroupMemberTag>();
        m_Members.clear();

        auto view = registry.view<ShaderCanvasComponent>();
        for (entt::entity entity : view)
        {
            ShaderCanvasComponent &canvas = view.get<ShaderCanvasComponent>(entity);
            if (canvas.shader.empty())
            {
                canvas.effect.reset();
                canvas.error.clear();
                continue;
            }
            if (!canvas.effect || canvas.effect->GetPath() != canvas.shader)
            {
                canvas.effect = ShaderEffect::Load(canvas.shader);
                canvas.effectVersion = 0;
            }
            else
                canvas.effect = ShaderEffect::Load(canvas.shader);
            canvas.error = canvas.effect->GetError();
            if (canvas.effect->IsValid() && canvas.effectVersion != canvas.effect->GetVersion())
            {
                SyncUniforms(canvas);
                canvas.effectVersion = canvas.effect->GetVersion();
            }
            else if (!canvas.effect->IsValid())
                canvas.effectVersion = 0;

            if (!apply || !canvas.enabled || !canvas.effect->IsValid() || canvas.scope == 0)
                continue;

            std::vector<entt::entity> members;
            if (canvas.scope == 1)
                CollectDescendants(registry, entity, members);
            else
            {
                for (entt::entity candidate : registry.view<TransformComponent>())
                {
                    if (candidate == entity || registry.any_of<ShaderCanvasComponent, CanvasComponent, UIRectComponent>(candidate))
                        continue;
                    const float z = registry.get<TransformComponent>(candidate).zIndex;
                    if (z >= canvas.zMin && z <= canvas.zMax)
                        members.push_back(candidate);
                }
            }

            std::vector<entt::entity> accepted;
            for (entt::entity member : members)
            {
                if (registry.all_of<ShaderGroupMemberTag>(member) || registry.all_of<CameraComponent>(member))
                    continue;
                registry.emplace<ShaderGroupMemberTag>(member);
                accepted.push_back(member);
            }
            m_Members[entity] = std::move(accepted);
        }
    }

    std::vector<entt::entity> ShaderCanvasSystem::Ordered(entt::registry &registry, bool groups) const
    {
        std::vector<entt::entity> result;
        for (entt::entity entity : registry.view<ShaderCanvasComponent>())
        {
            const ShaderCanvasComponent &canvas = registry.get<ShaderCanvasComponent>(entity);
            if (!canvas.enabled || !canvas.effect || !canvas.effect->IsValid())
                continue;
            if ((canvas.scope != 0) != groups)
                continue;
            result.push_back(entity);
        }
        std::stable_sort(result.begin(), result.end(), [&](entt::entity a, entt::entity b)
                         { return registry.get<ShaderCanvasComponent>(a).order < registry.get<ShaderCanvasComponent>(b).order; });
        return result;
    }

    bool ShaderCanvasSystem::AreaPixels(Scene &scene, entt::entity entity, const ShaderCanvasComponent &canvas, const glm::vec2 &viewport, int rect[4]) const
    {
        entt::registry &registry = scene.GetRegistry();
        if (!canvas.limitArea || entity == entt::null || !registry.valid(entity) || !registry.all_of<TransformComponent>(entity))
            return false;

        const WorldTransform2D world = DecomposeWorld(registry.get<TransformComponent>(entity));
        const glm::vec2 half = canvas.areaSize * glm::abs(world.Scale) * 0.5f;
        const glm::mat4 viewProjection = Renderer::GetAPI()->GetProjectionMatrix() * Renderer::GetViewMatrix();
        const glm::vec4 a = viewProjection * glm::vec4(world.Position - half, 0.0f, 1.0f);
        const glm::vec4 b = viewProjection * glm::vec4(world.Position + half, 0.0f, 1.0f);
        const float x0 = (std::min(a.x, b.x) * 0.5f + 0.5f) * viewport.x;
        const float x1 = (std::max(a.x, b.x) * 0.5f + 0.5f) * viewport.x;
        const float y0 = (std::min(a.y, b.y) * 0.5f + 0.5f) * viewport.y;
        const float y1 = (std::max(a.y, b.y) * 0.5f + 0.5f) * viewport.y;

        const int left = std::clamp(static_cast<int>(std::floor(x0)), 0, static_cast<int>(viewport.x));
        const int right = std::clamp(static_cast<int>(std::ceil(x1)), 0, static_cast<int>(viewport.x));
        const int bottom = std::clamp(static_cast<int>(std::floor(y0)), 0, static_cast<int>(viewport.y));
        const int top = std::clamp(static_cast<int>(std::ceil(y1)), 0, static_cast<int>(viewport.y));
        rect[0] = left;
        rect[1] = bottom;
        rect[2] = right - left;
        rect[3] = top - bottom;
        return true;
    }

    void ShaderCanvasSystem::Composite(Scene &scene, ShaderCanvasComponent &canvas, unsigned int sourceTexture, bool blend, int width, int height, const int *area)
    {
        if (area && (area[2] <= 0 || area[3] <= 0))
            return;

        ShaderEffect &effect = *canvas.effect;
        EnsureVao();

        glBindVertexArray(s_Vao);
        glUseProgram(effect.GetProgram());
        glDisable(GL_DEPTH_TEST);
        if (blend)
        {
            glEnable(GL_BLEND);
            glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        }
        else
            glDisable(GL_BLEND);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sourceTexture);
        glUniform1i(effect.Location("uTexture"), 0);
        glUniform1f(effect.Location("uTime"), scene.GetTime());
        glUniform2f(effect.Location("uResolution"), static_cast<float>(width), static_cast<float>(height));
        const glm::vec2 viewport = Input::GetViewportSize();
        const glm::vec2 mouse = Input::GetMouseViewportPosition();
        const float mx = viewport.x > 0.0f ? std::clamp(mouse.x / viewport.x, 0.0f, 1.0f) : 0.0f;
        const float my = viewport.y > 0.0f ? std::clamp(1.0f - mouse.y / viewport.y, 0.0f, 1.0f) : 0.0f;
        glUniform2f(effect.Location("uMouse"), mx, my);
        glUniform1f(effect.Location("uProgress"), canvas.progress);
        const glm::vec2 target = Renderer::GetAPI()->GetViewportSize();
        if (area && target.x > 0.0f && target.y > 0.0f)
            glUniform4f(effect.Location("uRegion"), area[0] / target.x, area[1] / target.y, area[2] / target.x, area[3] / target.y);
        else
            glUniform4f(effect.Location("uRegion"), 0.0f, 0.0f, 1.0f, 1.0f);
        if (area)
        {
            glEnable(GL_SCISSOR_TEST);
            glScissor(area[0], area[1], area[2], area[3]);
        }

        int unit = 1;
        for (const ShaderUniformValue &uniform : canvas.uniforms)
        {
            const int location = effect.Location(uniform.name);
            if (location < 0)
                continue;
            switch (static_cast<ShaderUniformType>(uniform.type))
            {
            case ShaderUniformType::Float:
                glUniform1f(location, uniform.value[0]);
                break;
            case ShaderUniformType::Vec2:
                glUniform2f(location, uniform.value[0], uniform.value[1]);
                break;
            case ShaderUniformType::Vec3:
                glUniform3f(location, uniform.value[0], uniform.value[1], uniform.value[2]);
                break;
            case ShaderUniformType::Vec4:
            case ShaderUniformType::Color:
                glUniform4f(location, uniform.value[0], uniform.value[1], uniform.value[2], uniform.value[3]);
                break;
            case ShaderUniformType::Int:
                glUniform1i(location, static_cast<int>(uniform.value[0]));
                break;
            case ShaderUniformType::Bool:
                glUniform1i(location, uniform.value[0] != 0.0f ? 1 : 0);
                break;
            case ShaderUniformType::Texture:
                glActiveTexture(GL_TEXTURE0 + unit);
                glBindTexture(GL_TEXTURE_2D, uniform.texture ? uniform.texture->GetID() : 0);
                glUniform1i(location, unit);
                ++unit;
                break;
            }
        }
        glActiveTexture(GL_TEXTURE0);

        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDisable(GL_SCISSOR_TEST);

        for (int i = unit - 1; i >= 1; --i)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
        glBindVertexArray(0);
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

    void ShaderCanvasSystem::RenderGroups(Scene &scene)
    {
        entt::registry &registry = scene.GetRegistry();
        const std::vector<entt::entity> canvases = Ordered(registry, true);
        if (canvases.empty())
            return;

        for (entt::entity entity : canvases)
        {
            auto found = m_Members.find(entity);
            if (found == m_Members.end() || found->second.empty())
                continue;
            ShaderCanvasComponent &canvas = registry.get<ShaderCanvasComponent>(entity);

            Renderer::Flush();
            RenderingAPI *api = Renderer::GetAPI();
            Framebuffer *target = api->GetRenderTarget();
            const glm::vec2 viewport = api->GetViewportSize();
            const float scale = std::clamp(canvas.resolutionScale, 0.1f, 2.0f);
            const int width = std::max(1, static_cast<int>(viewport.x * scale));
            const int height = std::max(1, static_cast<int>(viewport.y * scale));

            auto &buffer = m_Groups[entity];
            if (!buffer)
                buffer = std::make_unique<Framebuffer>(width, height, false);
            else if (buffer->GetWidth() != width || buffer->GetHeight() != height)
                buffer->Resize(width, height);

            Renderer::BeginRenderTarget(*buffer);
            Renderer::SetClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            Renderer::Clear();
            for (entt::entity member : found->second)
                if (registry.valid(member))
                    RenderSystem::DrawEntity(registry, member);
            Renderer::Flush();

            if (target)
                Renderer::BeginRenderTarget(*target);
            else
                Renderer::EndRenderTarget();

            int area[4] = {0, 0, 0, 0};
            const bool limited = AreaPixels(scene, entity, canvas, viewport, area);
            Composite(scene, canvas, buffer->GetColorAttachment(), true, width, height, limited ? area : nullptr);
        }
    }

    void ShaderCanvasSystem::RenderOne(Scene &scene, ShaderCanvasComponent &canvas, entt::entity entity)
    {
        Renderer::Flush();
        RenderingAPI *api = Renderer::GetAPI();
        Framebuffer *target = api->GetRenderTarget();
        const glm::vec2 viewport = api->GetViewportSize();
        const int width = std::max(1, static_cast<int>(viewport.x));
        const int height = std::max(1, static_cast<int>(viewport.y));
        if (width < 4 || height < 4)
            return;

        auto &copy = m_Copies[target];
        if (!copy)
            copy = std::make_unique<Framebuffer>(width, height, target && target->IsHDR());
        else if (copy->GetWidth() != width || copy->GetHeight() != height)
            copy->Resize(width, height);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, target ? target->GetID() : 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, copy->GetID());
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        if (target)
            Renderer::BeginRenderTarget(*target);
        else
            Renderer::EndRenderTarget();

        int area[4] = {0, 0, 0, 0};
        const bool limited = AreaPixels(scene, entity, canvas, viewport, area);
        Composite(scene, canvas, copy->GetColorAttachment(), false, width, height, limited ? area : nullptr);
    }

    void ShaderCanvasSystem::RenderPost(Scene &scene)
    {
        entt::registry &registry = scene.GetRegistry();
        for (entt::entity entity : Ordered(registry, false))
            RenderOne(scene, registry.get<ShaderCanvasComponent>(entity), entity);
    }
}
