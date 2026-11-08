#pragma once

#include <cstdint>

struct lua_State;

namespace SIMPEngine
{
    class VectorPath;
    class Curve;
    class TweenSystem;

    namespace LuaBindings
    {
        void RegisterTween(lua_State *L);
        void RegisterPath(lua_State *L);
        void RegisterDraw(lua_State *L);

        VectorPath *TestPath(lua_State *L, int index);
        void PushPath(lua_State *L, const VectorPath &path);
        int PushTween(lua_State *L, uint64_t serial, uint32_t id);
        bool ReadCurve(lua_State *L, int index, Curve &out);
        void ApplyTweenOptions(lua_State *L, int index, TweenSystem &system, uint32_t id);

        void BeginDraw(float zIndex);
        void EndDraw();
    }
}
