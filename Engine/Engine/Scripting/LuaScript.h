#pragma once

#include <Engine/Scene/ScriptableEntity.h>

#include <cstdint>
#include <string>

namespace SIMPEngine
{
    class LuaScript : public ScriptableEntity
    {
    public:
        explicit LuaScript(std::string path);
        ~LuaScript() override;

        const std::string &GetPath() const { return m_Path; }
        bool HasFailed() const { return m_Failed; }

        static constexpr int NoRef = -2;

    protected:
        void OnCreate() override;
        void OnDestroy() override;
        void OnUpdate(float dt) override;
        void OnRender() override;

    private:
        bool IsLive() const;
        bool HasHook(const char *name) const;
        bool Invoke(const char *hook, bool passDelta, float dt);
        void Disable();

        std::string m_Path;
        uint64_t m_Generation = 0;
        int m_SelfRef = NoRef;
        bool m_HasUpdate = false;
        bool m_HasRender = false;
        bool m_Failed = false;
    };
}
