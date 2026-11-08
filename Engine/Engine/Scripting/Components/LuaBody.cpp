#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *BodyMeta = "SIMP.Body";

        AccelEngine::RigidBody *CheckBody(lua_State *L)
        {
            PhysicsComponent *physics = CheckComponent<PhysicsComponent>(L, BodyMeta);
            if (!physics->body)
                luaL_error(L, "rigid body has not been created");
            return physics->body.get();
        }

        int BodyGetVelocity(lua_State *L)
        {
            const AccelEngine::RigidBody *body = CheckBody(L);
            lua_pushnumber(L, body->velocity.x);
            lua_pushnumber(L, body->velocity.y);
            return 2;
        }

        int BodySetVelocity(lua_State *L)
        {
            AccelEngine::RigidBody *body = CheckBody(L);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            body->velocity = {x, y};
            return 0;
        }

        int BodyAddImpulse(lua_State *L)
        {
            AccelEngine::RigidBody *body = CheckBody(L);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            const float inverseMass = body->inverseMass;
            if (inverseMass > 0.0f)
                body->velocity = {body->velocity.x + x * inverseMass, body->velocity.y + y * inverseMass};
            return 0;
        }

        int BodyGetSpin(lua_State *L)
        {
            lua_pushnumber(L, CheckBody(L)->rotation);
            return 1;
        }

        int BodySetSpin(lua_State *L)
        {
            AccelEngine::RigidBody *body = CheckBody(L);
            body->rotation = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int BodyIsStatic(lua_State *L)
        {
            lua_pushboolean(L, CheckBody(L)->inverseMass <= 0.0f);
            return 1;
        }

        int BodyGetMass(lua_State *L)
        {
            const float inverseMass = CheckBody(L)->inverseMass;
            lua_pushnumber(L, inverseMass > 0.0f ? 1.0f / inverseMass : 0.0f);
            return 1;
        }

        const luaL_Reg BodyMethods[] = {
            {"GetVelocity", BodyGetVelocity},
            {"SetVelocity", BodySetVelocity},
            {"AddImpulse", BodyAddImpulse},
            {"GetSpin", BodyGetSpin},
            {"SetSpin", BodySetSpin},
            {"IsStatic", BodyIsStatic},
            {"GetMass", BodyGetMass},
            {nullptr, nullptr}};
    }

    const ComponentBinding &BodyBinding()
    {
        static const ComponentBinding binding{"Body", BodyMeta, BodyMethods, &HasOf<PhysicsComponent>, nullptr, &RemoveOf<PhysicsComponent>};
        return binding;
    }
}
