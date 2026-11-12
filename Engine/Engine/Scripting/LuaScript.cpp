#include "LuaScript.h"
#include "LuaBindings.h"
#include "LuaInclude.h"
#include "LuaModules.h"
#include "LuaRuntime.h"

#include <Engine/Core/Log.h>
#include <Engine/Scene/Component.h>

namespace SIMPEngine
{
    static_assert(LuaScript::NoRef == LUA_NOREF);
}

namespace
{
    constexpr const char *EventHooks[] = {
        "OnFixedUpdate",
        "OnTriggerEnter",
        "OnTriggerStay",
        "OnTriggerExit",
        "OnCollision",
        "OnTimer",
        "OnDamaged",
        "OnDeath",
        "OnPathPoint",
        "OnPathEnd",
        "OnLand",
        "OnJump",
        "OnGrab",
        "OnDrop",
        "OnClick",
        "OnHoverEnter",
        "OnHoverExit"};

    static_assert(sizeof(EventHooks) / sizeof(EventHooks[0]) == static_cast<size_t>(SIMPEngine::ScriptEvent::Count));

    int CallHook(lua_State *L)
    {
        const char *name = lua_tostring(L, 2);
        const int extra = lua_gettop(L) - 2;
        lua_getfield(L, 1, name);
        if (!lua_isfunction(L, -1))
            return 0;

        lua_pushvalue(L, 1);
        for (int i = 0; i < extra; ++i)
            lua_pushvalue(L, 3 + i);
        lua_call(L, extra + 1, 0);
        return 0;
    }

    int ProbeHook(lua_State *L)
    {
        const char *name = lua_tostring(L, 2);
        lua_getfield(L, 1, name);
        lua_pushboolean(L, lua_isfunction(L, -1));
        return 1;
    }
}

namespace SIMPEngine
{
    LuaScript::LuaScript(std::string path) : m_Path(std::move(path))
    {
    }

    LuaScript::~LuaScript()
    {
        if (!IsLive())
            return;

        luaL_unref(LuaRuntime::Get().GetState(), LUA_REGISTRYINDEX, m_SelfRef);
        m_SelfRef = NoRef;
    }

    bool LuaScript::IsLive() const
    {
        const LuaRuntime &runtime = LuaRuntime::Get();
        return m_SelfRef != NoRef && runtime.IsInitialized() && runtime.GetGeneration() == m_Generation;
    }

    bool LuaScript::PushSelf(lua_State *L) const
    {
        if (!IsLive())
        {
            lua_pushnil(L);
            return false;
        }

        lua_rawgeti(L, LUA_REGISTRYINDEX, m_SelfRef);
        return true;
    }

    bool LuaScript::HasHook(const char *name) const
    {
        LuaRuntime &runtime = LuaRuntime::Get();
        lua_State *L = runtime.GetState();
        const int top = lua_gettop(L);

        lua_pushcfunction(L, ProbeHook);
        lua_rawgeti(L, LUA_REGISTRYINDEX, m_SelfRef);
        lua_pushstring(L, name);

        bool found = false;
        if (runtime.Call(2, 1))
            found = lua_toboolean(L, -1) != 0;

        lua_settop(L, top);
        return found;
    }

    bool LuaScript::Invoke(const char *hook, bool hasNumber, float number, const Entity &other)
    {
        LuaRuntime &runtime = LuaRuntime::Get();
        lua_State *L = runtime.GetState();
        const int top = lua_gettop(L);
        LuaBindings::SceneScope scope(GetEntity().GetScene());

        lua_pushcfunction(L, CallHook);
        lua_rawgeti(L, LUA_REGISTRYINDEX, m_SelfRef);
        lua_pushstring(L, hook);
        int argCount = 2;
        if (hasNumber)
        {
            lua_pushnumber(L, number);
            ++argCount;
        }
        if (static_cast<bool>(other))
        {
            LuaBindings::PushEntity(L, other);
            ++argCount;
        }

        const bool ok = runtime.Call(argCount, 0);
        lua_settop(L, top);
        return ok;
    }

    void LuaScript::Disable()
    {
        m_Failed = true;
        CORE_ERROR("Lua: script '{}' disabled after an error", m_Path);
    }

    void LuaScript::OnCreate()
    {
        LuaRuntime &runtime = LuaRuntime::Get();
        if (!runtime.PushScriptClass(m_Path))
        {
            Disable();
            return;
        }

        lua_State *L = runtime.GetState();

        lua_newtable(L);
        lua_pushvalue(L, -2);
        lua_setmetatable(L, -2);
        LuaBindings::PushEntity(L, GetEntity());
        lua_setfield(L, -2, "entity");
        lua_pushvalue(L, -1);
        m_SelfRef = luaL_ref(L, LUA_REGISTRYINDEX);
        m_Generation = runtime.GetGeneration();
        lua_pop(L, 2);

        m_HasUpdate = HasHook("OnUpdate");
        m_HasRender = HasHook("OnRender");
        for (size_t i = 0; i < m_EventHooks.size(); ++i)
            m_EventHooks[i] = HasHook(EventHooks[i]);

        if (!Invoke("OnCreate", false, 0.0f, Entity{}))
            Disable();
    }

    void LuaScript::OnDestroy()
    {
        if (m_Failed || !IsLive())
            return;

        Invoke("OnDestroy", false, 0.0f, Entity{});
    }

    void LuaScript::OnUpdate(float dt)
    {
        if (m_Failed || !m_HasUpdate || !IsLive())
            return;

        if (!Invoke("OnUpdate", true, dt, Entity{}))
            Disable();
    }

    void LuaScript::OnRender()
    {
        if (m_Failed || !m_HasRender || !IsLive())
            return;

        float z = 0.0f;
        if (Entity entity = GetEntity(); entity && entity.HasComponent<TransformComponent>())
            z = entity.GetComponent<TransformComponent>().zIndex;
        LuaBindings::BeginDraw(z);
        const bool ok = Invoke("OnRender", false, 0.0f, Entity{});
        LuaBindings::EndDraw();
        if (!ok)
            Disable();
    }

    void LuaScript::OnEvent(const ScriptMessage &message)
    {
        const size_t index = static_cast<size_t>(message.Type);
        if (m_Failed || index >= m_EventHooks.size() || !m_EventHooks[index] || !IsLive())
            return;

        bool hasNumber = false;
        float number = message.Value;
        bool passOther = false;

        switch (message.Type)
        {
        case ScriptEvent::FixedUpdate:
            hasNumber = true;
            break;
        case ScriptEvent::Damaged:
            hasNumber = true;
            passOther = true;
            break;
        case ScriptEvent::PathPoint:
            hasNumber = true;
            number += 1.0f;
            break;
        case ScriptEvent::TriggerEnter:
        case ScriptEvent::TriggerStay:
        case ScriptEvent::TriggerExit:
        case ScriptEvent::Collision:
        case ScriptEvent::Death:
            passOther = true;
            break;
        case ScriptEvent::Timer:
        case ScriptEvent::PathEnd:
        case ScriptEvent::Jumped:
        case ScriptEvent::Grabbed:
        case ScriptEvent::Dropped:
        case ScriptEvent::Clicked:
        case ScriptEvent::HoverEnter:
        case ScriptEvent::HoverExit:
        case ScriptEvent::Count:
            break;
        case ScriptEvent::Landed:
            hasNumber = true;
            break;
        }

        if (!Invoke(EventHooks[index], hasNumber, number, passOther ? message.Other : Entity{}))
            Disable();
    }
}

ScriptComponent::ScriptComponent() = default;
ScriptComponent::~ScriptComponent() = default;
ScriptComponent::ScriptComponent(ScriptComponent &&) noexcept = default;
ScriptComponent &ScriptComponent::operator=(ScriptComponent &&) noexcept = default;

void ScriptComponent::BindLua(const std::string &path)
{
    LuaPath = path;
    InstantiateScript = [path]() -> std::unique_ptr<SIMPEngine::ScriptableEntity>
    {
        return std::make_unique<SIMPEngine::LuaScript>(path);
    };
}

void ScriptComponent::ClearScript()
{
    Instance.reset();
    InstantiateScript = nullptr;
    LuaPath.clear();
}
