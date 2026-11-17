#include "MotionPreview.h"

#include "UI/Theme.h"
#include "UI/Widgets.h"

#include <Engine/Animation/EffectPresets.h>
#include <Engine/Rendering/Framebuffer.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scripting/LuaBindings.h>
#include <Engine/Scripting/LuaRuntime.h>

#include <Engine/Scripting/LuaInclude.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

namespace
{
    constexpr const char *Runner = R"(
return function(code, subject)
    local used = false
    local function touch()
        used = true
        return subject
    end
    local self = setmetatable({}, {__index = function() return touch() end})
    local env = setmetatable({self = self}, {__index = function(_, k)
        local v = _G[k]
        if v ~= nil then return v end
        return touch()
    end})
    local fn, err = load(code, "=preview", "t", env)
    if not fn then error(err, 0) end
    fn()
    return used
end
)";

    void Budget(lua_State *L, lua_Debug *)
    {
        luaL_error(L, "the preview took too long");
    }

    entt::entity Subject(SIMPEngine::Scene &scene, bool square)
    {
        SIMPEngine::Entity e = scene.CreateEntity("Subject");
        auto &v = e.AddComponent<VectorShapeComponent>();
        v.path = square ? SIMPEngine::VectorPath::Rect(80.0f, 80.0f, 14.0f) : SIMPEngine::VectorPath::Circle(42.0f);
        v.fillColor = SDL_Color{92, 212, 214, 255};
        return e.GetHandle();
    }
}

float MotionViewHeight(const std::string &category)
{
    if (category == "Shapes" || category == "Morph" || category == "Exits" || category == "Loops")
        return 250.0f;
    if (category == "Text")
        return 260.0f;
    if (category == "UI" || category == "Game UI")
        return 340.0f;
    if (category == "Physics")
        return 380.0f;
    return 440.0f;
}

MotionPreview::MotionPreview() = default;
MotionPreview::~MotionPreview() = default;

void MotionPreview::ShowMotion(const std::string &name, const SIMPEngine::MotionOptions &options)
{
    const std::string key = "motion:" + name;
    if (key == m_Key && m_Mode == Mode::Motion)
        return;
    m_Key = key;
    m_Mode = Mode::Motion;
    m_Name = name;
    m_Options = options;
    Restart();
}

void MotionPreview::ShowEffect(const std::string &name)
{
    const std::string key = "effect:" + name;
    if (key == m_Key)
        return;
    m_Key = key;
    m_Mode = Mode::Effect;
    m_Name = name;
    Restart();
}

void MotionPreview::ShowLua(const std::string &code, const std::string &fallbackMotion, const std::string &fallbackEffect)
{
    const std::string key = "lua:" + code;
    if (key == m_Key)
        return;
    m_Key = key;
    m_Mode = Mode::Lua;
    m_HasBounds = false;
    m_ViewCenter = glm::vec2(0.0f);
    m_ViewScale = 1.0f;
    m_Code = code;
    m_FallbackMotion = fallbackMotion;
    m_FallbackEffect = fallbackEffect;
    Restart();
}

void MotionPreview::Release()
{
    m_Scene.reset();
    m_Target.reset();
    m_Key.clear();
    m_Mode = Mode::None;
    m_Time = 0.0f;
}

void MotionPreview::Restart()
{
    if (!m_Scene)
        m_Scene = std::make_unique<SIMPEngine::Scene>("Preview");
    else
        m_Scene->Clear();
    m_Time = 0.0f;
    m_Error.clear();
    m_UntilIdle = false;
    Build();
}

bool MotionPreview::RunLua()
{
    using namespace SIMPEngine;
    LuaRuntime &runtime = LuaRuntime::Get();
    if (!runtime.IsInitialized() && !runtime.Initialize())
    {
        m_Error = "Lua is not available";
        return false;
    }
    lua_State *L = runtime.GetState();
    const entt::entity subject = Subject(*m_Scene, false);
    LuaBindings::SceneScope scope(m_Scene.get());
    const int top = lua_gettop(L);
    bool ok = luaL_loadstring(L, Runner) == LUA_OK && lua_pcall(L, 0, 1, 0) == LUA_OK;
    if (ok)
    {
        lua_pushlstring(L, m_Code.data(), m_Code.size());
        LuaBindings::PushEntity(L, Entity(subject, m_Scene.get()));
        lua_sethook(L, Budget, LUA_MASKCOUNT, 200000);
        ok = lua_pcall(L, 2, 1, 0) == LUA_OK;
        lua_sethook(L, nullptr, 0, 0);
        if (ok && !lua_toboolean(L, -1) && m_Scene->GetRegistry().valid(subject))
            m_Scene->GetRegistry().destroy(subject);
    }
    if (!ok)
    {
        const char *message = lua_tostring(L, -1);
        m_Error = message ? message : "error";
    }
    lua_settop(L, top);
    return ok;
}

void MotionPreview::Build()
{
    using namespace SIMPEngine;
    m_Length = 1.0f;
    std::string motion, effect;
    if (m_Mode == Mode::Lua)
    {
        if (RunLua())
        {
            m_UntilIdle = true;
            m_Length = 8.0f;
            return;
        }
        if (m_FallbackMotion.empty() && m_FallbackEffect.empty())
            return;
        m_Scene->Clear();
        motion = m_FallbackMotion;
        effect = m_FallbackEffect;
    }
    else if (m_Mode == Mode::Motion)
        motion = m_Name;
    else if (m_Mode == Mode::Effect)
        effect = m_Name;

    if (const MotionPreset *preset = motion.empty() ? nullptr : MotionPresets::Find(motion))
    {
        float length = preset->Duration;
        MotionPresets::Play(*m_Scene, *preset, m_Options, &length);
        m_Length = length + 0.9f;
    }
    else if (const EffectPreset *preset = effect.empty() ? nullptr : EffectPresets::Find(effect))
    {
        const entt::entity subject = Subject(*m_Scene, true);
        EffectPresets::Play(m_Scene->GetTweens(), subject, *preset);
        m_Length = (preset->Loop ? 2.4f : preset->Duration) + 0.7f;
    }
}

void MotionPreview::Update(float dt)
{
    if (!m_Scene || m_Mode == Mode::None)
        return;
    m_Time += dt;
    m_Scene->GetTweens().Update(dt);
    if (m_UntilIdle)
    {
        if (!m_Scene->GetTweens().HasActive() && m_Time < m_Length)
            m_Length = m_Time + 0.7f;
    }
    if (m_Time >= m_Length)
        Restart();
}

ImTextureID MotionPreview::Render(int width, int height, float viewHeight)
{
    using namespace SIMPEngine;
    width = std::max(8, width);
    height = std::max(8, height);
    if (!m_Target)
        m_Target = std::make_unique<Framebuffer>(width, height, false, 4);
    else if (m_Target->GetWidth() != width || m_Target->GetHeight() != height)
        m_Target->Resize(width, height);

    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    glm::vec2 center{0.0f};
    if (m_Mode == Mode::Lua && m_Scene)
    {
        auto view = m_Scene->GetRegistry().view<TransformComponent>();
        for (auto entity : view)
        {
            const auto &tf = view.get<TransformComponent>(entity);
            glm::vec2 lo{-10.0f}, hi{10.0f};
            if (const auto *shape = m_Scene->GetRegistry().try_get<VectorShapeComponent>(entity))
                shape->path.GetBounds(lo, hi);
            else if (const auto *text = m_Scene->GetRegistry().try_get<TextComponent>(entity))
            {
                const float w = text->Size * 0.3f * static_cast<float>(text->Text.size());
                lo = {-w, -text->Size * 0.5f};
                hi = {w, text->Size * 0.5f};
            }
            else
                continue;
            for (const glm::vec2 corner : {lo, hi, glm::vec2(lo.x, hi.y), glm::vec2(hi.x, lo.y)})
            {
                const glm::vec4 world = tf.worldTransform * glm::vec4(corner, 0.0f, 1.0f);
                const glm::vec2 p(world.x, world.y);
                if (!m_HasBounds)
                {
                    m_BoundsMin = m_BoundsMax = p;
                    m_HasBounds = true;
                }
                m_BoundsMin = glm::min(m_BoundsMin, p);
                m_BoundsMax = glm::max(m_BoundsMax, p);
            }
        }
        if (m_HasBounds)
        {
            const glm::vec2 size = m_BoundsMax - m_BoundsMin;
            const float wanted = std::max(180.0f, std::max(size.y, size.x / aspect) * 1.25f);
            const glm::vec2 target = (m_BoundsMin + m_BoundsMax) * 0.5f;
            const float k = std::min(1.0f, ImGui::GetIO().DeltaTime * 8.0f);
            m_ViewCenter += (target - m_ViewCenter) * k;
            m_ViewScale += (wanted / viewHeight - m_ViewScale) * k;
        }
        center = m_ViewCenter;
        viewHeight *= m_ViewScale;
    }
    const float viewW = viewHeight * aspect;
    const glm::mat4 proj = glm::ortho(center.x - viewW * 0.5f, center.x + viewW * 0.5f, center.y - viewHeight * 0.5f, center.y + viewHeight * 0.5f, -100000.0f, 100000.0f);
    const ImVec4 &bg = EditorUI::Colors::Canvas;
    Renderer::BeginRenderTarget(*m_Target);
    Renderer::SetClearColor(bg.x * 0.8f, bg.y * 0.8f, bg.z * 0.85f, 1.0f);
    Renderer::Clear();
    Renderer::BeginScene(glm::mat4(1.0f), proj);
    if (m_Scene)
        m_Scene->OnRender(glm::mat4(1.0f), proj);
    Renderer::EndScene();
    Renderer::EndRenderTarget();
    m_Target->Resolve();
    return EditorUI::TextureId(m_Target->GetColorAttachment());
}
