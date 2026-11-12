#include "LuaComponents.h"

#include <Engine/Physics/BodyFactory.h>
#include <Engine/Scene/Systems/PhysicsSystem.h>
#include <Engine/Scripting/LuaHelpers.h>

#include <algorithm>
#include <cmath>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *BodyMeta = "SIMP.Body";

        AccelEngine::RigidBody *CheckBody(lua_State *L)
        {
            PhysicsComponent *physics = CheckComponent<PhysicsComponent>(L, BodyMeta);
            if (!physics->body)
                luaL_error(L, "rigid body has not been created (use CreateBox, CreateCircle, CreateCapsule or CreatePolygon)");
            return physics->body.get();
        }

        float Num(lua_State *L, int index)
        {
            return static_cast<float>(luaL_checknumber(L, index));
        }

        float NumOpt(lua_State *L, int index, float fallback)
        {
            return static_cast<float>(luaL_optnumber(L, index, fallback));
        }

        bool Bool(lua_State *L, int index)
        {
            luaL_checkany(L, index);
            return lua_toboolean(L, index) != 0;
        }

        bool Install(Entity *entity, std::unique_ptr<AccelEngine::RigidBody> body)
        {
            if (!body)
                return false;
            entity->GetScene()->AddBody(entity->GetHandle(), std::move(body));
            return true;
        }

        int Finish(lua_State *L, bool created)
        {
            if (!created)
                return luaL_error(L, "could not create a body from that shape");
            return 0;
        }

        int BodyCreateBox(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<PhysicsComponent>(L, BodyMeta);
            const float w = Num(L, 2), h = Num(L, 3);
            PhysicsBodies::Options options;
            options.Mass = NumOpt(L, 4, 1.0f);
            return Finish(L, Install(entity, PhysicsBodies::Box({w * 0.5f, h * 0.5f}, options)));
        }

        int BodyCreateCircle(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<PhysicsComponent>(L, BodyMeta);
            const float radius = Num(L, 2);
            PhysicsBodies::Options options;
            options.Mass = NumOpt(L, 3, 1.0f);
            return Finish(L, Install(entity, PhysicsBodies::Circle(radius, options)));
        }

        int BodyCreateCapsule(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<PhysicsComponent>(L, BodyMeta);
            const float halfLength = Num(L, 2), radius = Num(L, 3);
            PhysicsBodies::Options options;
            options.Mass = NumOpt(L, 4, 1.0f);
            return Finish(L, Install(entity, PhysicsBodies::Capsule(halfLength, radius, options)));
        }

        int BodyCreatePolygon(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<PhysicsComponent>(L, BodyMeta);
            const int sides = static_cast<int>(luaL_checkinteger(L, 2));
            const float radius = Num(L, 3);
            PhysicsBodies::Options options;
            options.Mass = NumOpt(L, 4, 1.0f);
            return Finish(L, Install(entity, PhysicsBodies::RegularPolygon(sides, radius, options)));
        }

        int BodyCreatePoints(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<PhysicsComponent>(L, BodyMeta);
            luaL_checktype(L, 2, LUA_TTABLE);
            const float cornerRadius = NumOpt(L, 3, 0.0f);
            PhysicsBodies::Options options;
            options.Mass = NumOpt(L, 4, 1.0f);

            bool created = false;
            {
                std::vector<glm::vec2> points;
                const lua_Integer count = static_cast<lua_Integer>(lua_rawlen(L, 2));
                for (lua_Integer i = 1; i <= count && i <= 64; ++i)
                {
                    lua_rawgeti(L, 2, i);
                    if (lua_type(L, -1) == LUA_TTABLE)
                    {
                        lua_rawgeti(L, -1, 1);
                        lua_rawgeti(L, -2, 2);
                        points.emplace_back(static_cast<float>(lua_tonumber(L, -2)), static_cast<float>(lua_tonumber(L, -1)));
                        lua_pop(L, 2);
                    }
                    lua_pop(L, 1);
                }
                created = Install(entity, PhysicsBodies::Polygon(points, cornerRadius, options));
            }
            return Finish(L, created);
        }

        int BodyDestroy(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<PhysicsComponent>(L, BodyMeta);
            entity->GetScene()->RemoveBody(entity->GetHandle());
            return 0;
        }

        int BodyHasShape(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<PhysicsComponent>(L, BodyMeta)->body != nullptr);
            return 1;
        }

        int BodyGetShape(lua_State *L)
        {
            lua_pushstring(L, PhysicsBodies::ShapeName(*CheckBody(L)));
            return 1;
        }

        int BodyGetSize(lua_State *L)
        {
            const glm::vec2 size = PhysicsBodies::ShapeExtents(*CheckBody(L));
            lua_pushnumber(L, size.x);
            lua_pushnumber(L, size.y);
            return 2;
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
            const float x = Num(L, 2), y = Num(L, 3);
            body->setVelocity({x, y}, body->rotation);
            return 0;
        }

        int BodyAddImpulse(lua_State *L)
        {
            AccelEngine::RigidBody *body = CheckBody(L);
            const float x = Num(L, 2), y = Num(L, 3);
            body->applyImpulse({x, y});
            return 0;
        }

        int BodyAddImpulseAt(lua_State *L)
        {
            AccelEngine::RigidBody *body = CheckBody(L);
            const float ix = Num(L, 2), iy = Num(L, 3), px = Num(L, 4), py = Num(L, 5);
            body->applyImpulseAtPoint({ix, iy}, {px, py});
            return 0;
        }

        float FrameTime(Entity *entity)
        {
            const float dt = entity->GetScene()->GetDeltaTime();
            return std::clamp(dt, 1.0f / 240.0f, 1.0f / 30.0f);
        }

        int BodyAddForce(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<PhysicsComponent>(L, BodyMeta);
            AccelEngine::RigidBody *body = CheckBody(L);
            const float x = Num(L, 2), y = Num(L, 3);
            const float dt = FrameTime(entity);
            body->applyImpulse({x * dt, y * dt});
            return 0;
        }

        int BodyAddTorque(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<PhysicsComponent>(L, BodyMeta);
            AccelEngine::RigidBody *body = CheckBody(L);
            const float torque = Num(L, 2);
            if (!body->isStatic())
            {
                body->rotation += torque * body->getEffectiveInverseInertia() * FrameTime(entity);
                body->wakeUp();
            }
            return 0;
        }

        int BodyStop(lua_State *L)
        {
            CheckBody(L)->setVelocity({0.0f, 0.0f}, 0.0f);
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
            body->setVelocity(body->velocity, Num(L, 2));
            return 0;
        }

        int BodyGetAngle(lua_State *L)
        {
            lua_pushnumber(L, glm::degrees(CheckBody(L)->orientation));
            return 1;
        }

        int BodyTeleport(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<PhysicsComponent>(L, BodyMeta);
            AccelEngine::RigidBody *body = CheckBody(L);
            const float x = Num(L, 2), y = Num(L, 3);
            const bool hasAngle = !lua_isnoneornil(L, 4);
            const float degrees = NumOpt(L, 4, 0.0f);
            TransformComponent &transform = entity->GetComponent<TransformComponent>();
            transform.position = {x, y};
            if (hasAngle)
                transform.rotation = degrees;
            transform.dirty = true;
            PhysicsSystem::SyncBodyToTransform(entity->GetScene()->GetRegistry(), entity->GetHandle());
            body->setVelocity({0.0f, 0.0f}, 0.0f);
            return 0;
        }

        int BodyIsStatic(lua_State *L)
        {
            lua_pushboolean(L, CheckBody(L)->isStatic());
            return 1;
        }

        int BodyGetMass(lua_State *L)
        {
            lua_pushnumber(L, CheckBody(L)->getMass());
            return 1;
        }

        int BodySetMass(lua_State *L)
        {
            AccelEngine::RigidBody *body = CheckBody(L);
            body->setMass(Num(L, 2));
            return 0;
        }

        int BodyGetBounce(lua_State *L)
        {
            lua_pushnumber(L, CheckBody(L)->restitution);
            return 1;
        }

        int BodySetBounce(lua_State *L)
        {
            AccelEngine::RigidBody *body = CheckBody(L);
            body->restitution = std::clamp(Num(L, 2), 0.0f, 1.0f);
            return 0;
        }

        int BodyGetFriction(lua_State *L)
        {
            const AccelEngine::RigidBody *body = CheckBody(L);
            lua_pushnumber(L, body->staticFriction);
            lua_pushnumber(L, body->dynamicFriction);
            return 2;
        }

        int BodySetFriction(lua_State *L)
        {
            AccelEngine::RigidBody *body = CheckBody(L);
            const float staticFriction = std::max(Num(L, 2), 0.0f);
            const float dynamicFriction = std::max(NumOpt(L, 3, staticFriction * 0.67f), 0.0f);
            body->staticFriction = staticFriction;
            body->dynamicFriction = dynamicFriction;
            return 0;
        }

        int BodySetDamping(lua_State *L)
        {
            AccelEngine::RigidBody *body = CheckBody(L);
            const float linear = Num(L, 2);
            const float angular = NumOpt(L, 3, linear);
            body->linearDamping = std::clamp(linear, 0.0f, 1.0f);
            body->angularDamping = std::clamp(angular, 0.0f, 1.0f);
            return 0;
        }

        int BodySetSensor(lua_State *L)
        {
            AccelEngine::RigidBody *body = CheckBody(L);
            body->isSensor = Bool(L, 2);
            body->wakeUp();
            return 0;
        }

        int BodyIsSensor(lua_State *L)
        {
            lua_pushboolean(L, CheckBody(L)->isSensor);
            return 1;
        }

        int BodySetOneWay(lua_State *L)
        {
            AccelEngine::RigidBody *body = CheckBody(L);
            const bool enabled = Bool(L, 2);
            const float nx = NumOpt(L, 3, 0.0f), ny = NumOpt(L, 4, 1.0f);
            body->oneWay = enabled;
            if (enabled && (nx != 0.0f || ny != 0.0f))
                body->oneWayNormal = AccelEngine::Vector2(nx, ny).normalized();
            body->wakeUp();
            return 0;
        }

        int BodyIsOneWay(lua_State *L)
        {
            lua_pushboolean(L, CheckBody(L)->oneWay);
            return 1;
        }

        int BodySetLocked(lua_State *L)
        {
            AccelEngine::RigidBody *body = CheckBody(L);
            const bool rotation = Bool(L, 2);
            const bool position = lua_isnoneornil(L, 3) ? body->lockPosition : lua_toboolean(L, 3) != 0;
            body->lockRotation = rotation;
            body->lockPosition = position;
            body->wakeUp();
            return 0;
        }

        int BodyIsLocked(lua_State *L)
        {
            const AccelEngine::RigidBody *body = CheckBody(L);
            lua_pushboolean(L, body->lockRotation);
            lua_pushboolean(L, body->lockPosition);
            return 2;
        }

        int BodySetIgnoreGravity(lua_State *L)
        {
            AccelEngine::RigidBody *body = CheckBody(L);
            body->ignoreGravity = Bool(L, 2);
            body->wakeUp();
            return 0;
        }

        int BodySetCanSleep(lua_State *L)
        {
            AccelEngine::RigidBody *body = CheckBody(L);
            body->canSleep = Bool(L, 2);
            if (!body->canSleep)
                body->wakeUp();
            return 0;
        }

        int BodyIsAwake(lua_State *L)
        {
            lua_pushboolean(L, CheckBody(L)->isAwake);
            return 1;
        }

        int BodyWake(lua_State *L)
        {
            CheckBody(L)->wakeUp();
            return 0;
        }

        int BodySleep(lua_State *L)
        {
            CheckBody(L)->setAwake(false);
            return 0;
        }

        int BodySetLayer(lua_State *L)
        {
            AccelEngine::RigidBody *body = CheckBody(L);
            const lua_Integer layer = luaL_checkinteger(L, 2);
            body->categoryBits = static_cast<uint16_t>(1u << std::clamp<lua_Integer>(layer, 0, 15));
            return 0;
        }

        int BodyGetLayer(lua_State *L)
        {
            const AccelEngine::RigidBody *body = CheckBody(L);
            int layer = 0;
            while (layer < 15 && !(body->categoryBits & (1u << layer)))
                ++layer;
            lua_pushinteger(L, layer);
            return 1;
        }

        int BodySetMask(lua_State *L)
        {
            AccelEngine::RigidBody *body = CheckBody(L);
            body->maskBits = static_cast<uint16_t>(luaL_checkinteger(L, 2) & 0xFFFF);
            return 0;
        }

        int BodyGetMask(lua_State *L)
        {
            lua_pushinteger(L, CheckBody(L)->maskBits);
            return 1;
        }

        int BodySetCollidesWith(lua_State *L)
        {
            AccelEngine::RigidBody *body = CheckBody(L);
            uint16_t mask = 0;
            const int top = lua_gettop(L);
            for (int i = 2; i <= top; ++i)
                mask |= static_cast<uint16_t>(1u << std::clamp<lua_Integer>(luaL_checkinteger(L, i), 0, 15));
            body->maskBits = mask;
            return 0;
        }

        int BodySetGroup(lua_State *L)
        {
            AccelEngine::RigidBody *body = CheckBody(L);
            body->groupIndex = static_cast<int16_t>(std::clamp<lua_Integer>(luaL_checkinteger(L, 2), -32768, 32767));
            return 0;
        }

        int BodyGetGroup(lua_State *L)
        {
            lua_pushinteger(L, CheckBody(L)->groupIndex);
            return 1;
        }

        const luaL_Reg BodyMethods[] = {
            {"CreateBox", BodyCreateBox},
            {"CreateCircle", BodyCreateCircle},
            {"CreateCapsule", BodyCreateCapsule},
            {"CreatePolygon", BodyCreatePolygon},
            {"CreatePoints", BodyCreatePoints},
            {"Destroy", BodyDestroy},
            {"HasShape", BodyHasShape},
            {"GetShape", BodyGetShape},
            {"GetSize", BodyGetSize},
            {"GetVelocity", BodyGetVelocity},
            {"SetVelocity", BodySetVelocity},
            {"AddImpulse", BodyAddImpulse},
            {"AddImpulseAt", BodyAddImpulseAt},
            {"AddForce", BodyAddForce},
            {"AddTorque", BodyAddTorque},
            {"Stop", BodyStop},
            {"GetSpin", BodyGetSpin},
            {"SetSpin", BodySetSpin},
            {"GetAngle", BodyGetAngle},
            {"Teleport", BodyTeleport},
            {"IsStatic", BodyIsStatic},
            {"GetMass", BodyGetMass},
            {"SetMass", BodySetMass},
            {"GetBounce", BodyGetBounce},
            {"SetBounce", BodySetBounce},
            {"GetFriction", BodyGetFriction},
            {"SetFriction", BodySetFriction},
            {"SetDamping", BodySetDamping},
            {"SetSensor", BodySetSensor},
            {"IsSensor", BodyIsSensor},
            {"SetOneWay", BodySetOneWay},
            {"IsOneWay", BodyIsOneWay},
            {"SetLocked", BodySetLocked},
            {"IsLocked", BodyIsLocked},
            {"SetIgnoreGravity", BodySetIgnoreGravity},
            {"SetCanSleep", BodySetCanSleep},
            {"IsAwake", BodyIsAwake},
            {"Wake", BodyWake},
            {"Sleep", BodySleep},
            {"SetLayer", BodySetLayer},
            {"GetLayer", BodyGetLayer},
            {"SetMask", BodySetMask},
            {"GetMask", BodyGetMask},
            {"SetCollidesWith", BodySetCollidesWith},
            {"SetGroup", BodySetGroup},
            {"GetGroup", BodyGetGroup},
            {nullptr, nullptr}};
    }

    const ComponentBinding &BodyBinding()
    {
        static const ComponentBinding binding{"Body", BodyMeta, BodyMethods, &HasOf<PhysicsComponent>, &AddOf<PhysicsComponent>, &RemoveOf<PhysicsComponent>};
        return binding;
    }
}
