#pragma once

#include <cstdint>
#include <memory>

struct lua_State;

namespace SIMPEngine
{
    class Scene;

    class LuaCallback
    {
    public:
        static std::shared_ptr<LuaCallback> FromStack(lua_State *L, int index);
        ~LuaCallback();

        LuaCallback(const LuaCallback &) = delete;
        LuaCallback &operator=(const LuaCallback &) = delete;

        void Call(Scene *scene) const;
        void Call(Scene *scene, float value) const;

    private:
        LuaCallback(int ref, uint64_t generation) : m_Ref(ref), m_Generation(generation) {}
        bool Push(lua_State *&L) const;

        int m_Ref = 0;
        uint64_t m_Generation = 0;
    };
}
