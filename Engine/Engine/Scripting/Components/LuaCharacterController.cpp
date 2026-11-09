#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

#include <cstring>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *ControllerMeta = "SIMP.CharacterController";

        CharacterControllerComponent *Check(lua_State *L)
        {
            return CheckComponent<CharacterControllerComponent>(L, ControllerMeta);
        }

        float CheckFloat(lua_State *L, int index)
        {
            return static_cast<float>(luaL_checknumber(L, index));
        }

        int GetVelocity(lua_State *L)
        {
            const CharacterControllerComponent *c = Check(L);
            lua_pushnumber(L, c->velocity.x);
            lua_pushnumber(L, c->velocity.y);
            return 2;
        }

        int SetVelocity(lua_State *L)
        {
            CharacterControllerComponent *c = Check(L);
            const float x = CheckFloat(L, 2);
            const float y = CheckFloat(L, 3);
            c->velocity = {x, y};
            return 0;
        }

        int SetInput(lua_State *L)
        {
            CharacterControllerComponent *c = Check(L);
            const float x = CheckFloat(L, 2);
            const float y = static_cast<float>(luaL_optnumber(L, 3, 0.0));
            c->scriptInput = {x, y};
            return 0;
        }

        int Jump(lua_State *L)
        {
            Check(L)->scriptJump = true;
            return 0;
        }

        int ReleaseJump(lua_State *L)
        {
            Check(L)->scriptRelease = true;
            return 0;
        }

        int IsGrounded(lua_State *L)
        {
            lua_pushboolean(L, Check(L)->grounded);
            return 1;
        }

        int JustLanded(lua_State *L)
        {
            lua_pushboolean(L, Check(L)->justLanded);
            return 1;
        }

        int JustJumped(lua_State *L)
        {
            lua_pushboolean(L, Check(L)->justJumped);
            return 1;
        }

        int HitWall(lua_State *L)
        {
            lua_pushboolean(L, Check(L)->hitWall);
            return 1;
        }

        int HitCeiling(lua_State *L)
        {
            lua_pushboolean(L, Check(L)->hitCeiling);
            return 1;
        }

        int GetFacing(lua_State *L)
        {
            lua_pushinteger(L, Check(L)->facing);
            return 1;
        }

        int GetPlayerInput(lua_State *L)
        {
            lua_pushboolean(L, Check(L)->playerInput);
            return 1;
        }

        int SetPlayerInput(lua_State *L)
        {
            CharacterControllerComponent *c = Check(L);
            c->playerInput = lua_toboolean(L, 2);
            return 0;
        }

        int GetMode(lua_State *L)
        {
            lua_pushstring(L, Check(L)->mode == ControllerMode::TopDown ? "topdown" : "platformer");
            return 1;
        }

        int SetMode(lua_State *L)
        {
            CharacterControllerComponent *c = Check(L);
            const char *mode = luaL_checkstring(L, 2);
            const bool topDown = std::strcmp(mode, "topdown") == 0 || std::strcmp(mode, "top_down") == 0;
            if (!topDown && std::strcmp(mode, "platformer") != 0)
                return luaL_error(L, "mode must be 'platformer' or 'topdown'");
            c->mode = topDown ? ControllerMode::TopDown : ControllerMode::Platformer;
            c->velocity = {0.0f, 0.0f};
            return 0;
        }

        int GetMaxJumps(lua_State *L)
        {
            lua_pushinteger(L, Check(L)->maxJumps);
            return 1;
        }

        int SetMaxJumps(lua_State *L)
        {
            CharacterControllerComponent *c = Check(L);
            const lua_Integer jumps = luaL_checkinteger(L, 2);
            c->maxJumps = static_cast<int>(jumps < 1 ? 1 : jumps);
            return 0;
        }

#define SIMP_CONTROLLER_FLOAT(Name, field)                       \
    int Get##Name(lua_State *L)                                  \
    {                                                            \
        lua_pushnumber(L, Check(L)->field);                      \
        return 1;                                                \
    }                                                            \
    int Set##Name(lua_State *L)                                  \
    {                                                            \
        CharacterControllerComponent *c = Check(L);              \
        const float value = CheckFloat(L, 2);                    \
        c->field = value;                                        \
        return 0;                                                \
    }

        SIMP_CONTROLLER_FLOAT(MoveSpeed, moveSpeed)
        SIMP_CONTROLLER_FLOAT(Acceleration, acceleration)
        SIMP_CONTROLLER_FLOAT(Deceleration, deceleration)
        SIMP_CONTROLLER_FLOAT(AirControl, airControl)
        SIMP_CONTROLLER_FLOAT(Gravity, gravity)
        SIMP_CONTROLLER_FLOAT(MaxFallSpeed, maxFallSpeed)
        SIMP_CONTROLLER_FLOAT(JumpSpeed, jumpSpeed)
        SIMP_CONTROLLER_FLOAT(JumpCut, jumpCut)
        SIMP_CONTROLLER_FLOAT(CoyoteTime, coyoteTime)
        SIMP_CONTROLLER_FLOAT(JumpBuffer, jumpBuffer)

#undef SIMP_CONTROLLER_FLOAT

        const luaL_Reg ControllerMethods[] = {
            {"GetVelocity", GetVelocity},
            {"SetVelocity", SetVelocity},
            {"SetInput", SetInput},
            {"Jump", Jump},
            {"ReleaseJump", ReleaseJump},
            {"IsGrounded", IsGrounded},
            {"JustLanded", JustLanded},
            {"JustJumped", JustJumped},
            {"HitWall", HitWall},
            {"HitCeiling", HitCeiling},
            {"GetFacing", GetFacing},
            {"GetPlayerInput", GetPlayerInput},
            {"SetPlayerInput", SetPlayerInput},
            {"GetMode", GetMode},
            {"SetMode", SetMode},
            {"GetMaxJumps", GetMaxJumps},
            {"SetMaxJumps", SetMaxJumps},
            {"GetMoveSpeed", GetMoveSpeed},
            {"SetMoveSpeed", SetMoveSpeed},
            {"GetAcceleration", GetAcceleration},
            {"SetAcceleration", SetAcceleration},
            {"GetDeceleration", GetDeceleration},
            {"SetDeceleration", SetDeceleration},
            {"GetAirControl", GetAirControl},
            {"SetAirControl", SetAirControl},
            {"GetGravity", GetGravity},
            {"SetGravity", SetGravity},
            {"GetMaxFallSpeed", GetMaxFallSpeed},
            {"SetMaxFallSpeed", SetMaxFallSpeed},
            {"GetJumpSpeed", GetJumpSpeed},
            {"SetJumpSpeed", SetJumpSpeed},
            {"GetJumpCut", GetJumpCut},
            {"SetJumpCut", SetJumpCut},
            {"GetCoyoteTime", GetCoyoteTime},
            {"SetCoyoteTime", SetCoyoteTime},
            {"GetJumpBuffer", GetJumpBuffer},
            {"SetJumpBuffer", SetJumpBuffer},
            {nullptr, nullptr}};
    }

    const ComponentBinding &CharacterControllerBinding()
    {
        static const ComponentBinding binding{"CharacterController", ControllerMeta, ControllerMethods, &HasOf<CharacterControllerComponent>, &AddOf<CharacterControllerComponent>, &RemoveOf<CharacterControllerComponent>};
        return binding;
    }
}
