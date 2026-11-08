#pragma once

#include <Engine/Scene/ScriptableEntity.h>

#include <array>
#include <cstdint>
#include <string>

struct lua_State;

namespace SIMPEngine
{
    class LuaScript : public ScriptableEntity
    {
    public:
        explicit LuaScript(std::string path);
        ~LuaScript() override;

        const std::string &GetPath() const { return m_Path; }
        bool HasFailed() const { return m_Failed; }
        bool PushSelf(lua_State *L) const;

        static constexpr int NoRef = -2;

    protected:
        void OnCreate() override;
        void OnDestroy() override;
        void OnUpdate(float dt) override;
        void OnRender() override;
        void OnEvent(const ScriptMessage &message) override;

    private:
        bool IsLive() const;
        bool HasHook(const char *name) const;
        bool Invoke(const char *hook, bool hasNumber, float number, const Entity &other);
        void Disable();

        std::string m_Path;
        uint64_t m_Generation = 0;
        int m_SelfRef = NoRef;
        bool m_HasUpdate = false;
        bool m_HasRender = false;
        bool m_Failed = false;
        std::array<bool, static_cast<size_t>(ScriptEvent::Count)> m_EventHooks{};
    };
}
