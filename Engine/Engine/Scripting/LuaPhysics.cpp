#include "LuaBindings.h"
#include "LuaHelpers.h"
#include "LuaInclude.h"
#include "LuaModules.h"

#include <Engine/Scene/Scene.h>

#include <AccelEngine/joint.h>
#include <AccelEngine/world.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *JointMeta = "SIMP.Joint";

        struct JointHandle
        {
            AccelEngine::Joint *Joint = nullptr;
        };

        AccelEngine::World &WorldOf(lua_State *L)
        {
            return RequireScene(L)->GetPhysicsWorld();
        }

        float Num(lua_State *L, int index)
        {
            return static_cast<float>(luaL_checknumber(L, index));
        }

        float NumOpt(lua_State *L, int index, float fallback)
        {
            return static_cast<float>(luaL_optnumber(L, index, fallback));
        }

        AccelEngine::RigidBody *BodyOf(lua_State *L, int index)
        {
            Entity *entity = CheckLiveEntity(L, index);
            if (!entity->HasComponent<PhysicsComponent>() || !entity->GetComponent<PhysicsComponent>().body)
            {
                luaL_error(L, "argument %d: the entity has no rigid body", index);
                return nullptr;
            }
            return entity->GetComponent<PhysicsComponent>().body.get();
        }

        void PushBodyEntity(lua_State *L, const AccelEngine::RigidBody *body)
        {
            PushEntity(L, Entity(static_cast<entt::entity>(body->entityID), RequireScene(L)));
        }

        int PhysicsSetGravity(lua_State *L)
        {
            const float x = Num(L, 1), y = Num(L, 2);
            WorldOf(L).gravity = {x, y};
            WorldOf(L).wakeAll();
            return 0;
        }

        int PhysicsGetGravity(lua_State *L)
        {
            const AccelEngine::Vector2 g = WorldOf(L).gravity;
            lua_pushnumber(L, g.x);
            lua_pushnumber(L, g.y);
            return 2;
        }

        int PhysicsSetSubsteps(lua_State *L)
        {
            WorldOf(L).settings.substeps = static_cast<int>(std::clamp<lua_Integer>(luaL_checkinteger(L, 1), 1, 64));
            return 0;
        }

        int PhysicsGetSubsteps(lua_State *L)
        {
            lua_pushinteger(L, WorldOf(L).settings.substeps);
            return 1;
        }

        int PhysicsSetIterations(lua_State *L)
        {
            const lua_Integer velocity = luaL_checkinteger(L, 1);
            const lua_Integer position = luaL_optinteger(L, 2, WorldOf(L).settings.positionIterations);
            WorldOf(L).settings.velocityIterations = static_cast<int>(std::clamp<lua_Integer>(velocity, 1, 64));
            WorldOf(L).settings.positionIterations = static_cast<int>(std::clamp<lua_Integer>(position, 0, 64));
            return 0;
        }

        int PhysicsSetSleepEnabled(lua_State *L)
        {
            luaL_checkany(L, 1);
            const bool enabled = lua_toboolean(L, 1) != 0;
            WorldOf(L).settings.enableSleep = enabled;
            if (!enabled)
                WorldOf(L).wakeAll();
            return 0;
        }

        int PhysicsWakeAll(lua_State *L)
        {
            WorldOf(L).wakeAll();
            return 0;
        }

        int PhysicsGetBodyCount(lua_State *L)
        {
            lua_pushinteger(L, WorldOf(L).getBodyCount());
            return 1;
        }

        int PhysicsGetAwakeCount(lua_State *L)
        {
            lua_pushinteger(L, WorldOf(L).getAwakeBodyCount());
            return 1;
        }

        int PhysicsRaycast(lua_State *L)
        {
            const float ox = Num(L, 1), oy = Num(L, 2), dx = Num(L, 3), dy = Num(L, 4);
            const float maxDistance = NumOpt(L, 5, 100000.0f);
            AccelEngine::RayHit hit;
            if (!WorldOf(L).raycast({ox, oy}, {dx, dy}, maxDistance, hit) || !hit.body)
            {
                lua_pushnil(L);
                return 1;
            }
            PushBodyEntity(L, hit.body);
            lua_pushnumber(L, hit.point.x);
            lua_pushnumber(L, hit.point.y);
            lua_pushnumber(L, hit.normal.x);
            lua_pushnumber(L, hit.normal.y);
            lua_pushnumber(L, hit.distance);
            return 6;
        }

        int PhysicsQueryPoint(lua_State *L)
        {
            const float x = Num(L, 1), y = Num(L, 2);
            const AccelEngine::RigidBody *body = WorldOf(L).queryPoint({x, y});
            if (!body)
            {
                lua_pushnil(L);
                return 1;
            }
            PushBodyEntity(L, body);
            return 1;
        }

        int PushBodies(lua_State *L, const std::vector<AccelEngine::RigidBody *> &bodies)
        {
            lua_createtable(L, static_cast<int>(bodies.size()), 0);
            lua_Integer index = 1;
            for (const AccelEngine::RigidBody *body : bodies)
            {
                PushBodyEntity(L, body);
                lua_rawseti(L, -2, index++);
            }
            return 1;
        }

        int PhysicsQueryBox(lua_State *L)
        {
            const float x1 = Num(L, 1), y1 = Num(L, 2), x2 = Num(L, 3), y2 = Num(L, 4);
            std::vector<AccelEngine::RigidBody *> found;
            WorldOf(L).queryAABB({std::min(x1, x2), std::min(y1, y2)}, {std::max(x1, x2), std::max(y1, y2)}, found);
            return PushBodies(L, found);
        }

        int PhysicsQueryCircle(lua_State *L)
        {
            const float x = Num(L, 1), y = Num(L, 2), radius = std::max(Num(L, 3), 0.0f);
            std::vector<AccelEngine::RigidBody *> found;
            WorldOf(L).queryAABB({x - radius, y - radius}, {x + radius, y + radius}, found);
            found.erase(std::remove_if(found.begin(), found.end(), [x, y, radius](const AccelEngine::RigidBody *body)
                                       {
                const float cx = std::clamp(x, body->worldAABBMin.x, body->worldAABBMax.x);
                const float cy = std::clamp(y, body->worldAABBMin.y, body->worldAABBMax.y);
                return (cx - x) * (cx - x) + (cy - y) * (cy - y) > radius * radius; }),
                        found.end());
            return PushBodies(L, found);
        }

        int PushJoint(lua_State *L, AccelEngine::Joint *joint)
        {
            JointHandle *handle = static_cast<JointHandle *>(lua_newuserdatauv(L, sizeof(JointHandle), 0));
            handle->Joint = joint;
            luaL_setmetatable(L, JointMeta);
            return 1;
        }

        int PhysicsCreateDistanceJoint(lua_State *L)
        {
            AccelEngine::RigidBody *a = BodyOf(L, 1);
            AccelEngine::RigidBody *b = BodyOf(L, 2);
            const float ax = NumOpt(L, 3, 0.0f), ay = NumOpt(L, 4, 0.0f), bx = NumOpt(L, 5, 0.0f), by = NumOpt(L, 6, 0.0f);
            const float length = NumOpt(L, 7, -1.0f);
            if (a == b)
                return luaL_error(L, "a joint needs two different bodies");
            return PushJoint(L, WorldOf(L).createDistanceJoint(a, b, {ax, ay}, {bx, by}, length));
        }

        int PhysicsCreateRevoluteJoint(lua_State *L)
        {
            AccelEngine::RigidBody *a = BodyOf(L, 1);
            AccelEngine::RigidBody *b = BodyOf(L, 2);
            const float x = Num(L, 3), y = Num(L, 4);
            if (a == b)
                return luaL_error(L, "a joint needs two different bodies");
            return PushJoint(L, WorldOf(L).createRevoluteJoint(a, b, {x, y}));
        }

        int PhysicsCreateWheelJoint(lua_State *L)
        {
            AccelEngine::RigidBody *a = BodyOf(L, 1);
            AccelEngine::RigidBody *b = BodyOf(L, 2);
            const float x = Num(L, 3), y = Num(L, 4);
            const float axisX = NumOpt(L, 5, 0.0f), axisY = NumOpt(L, 6, 1.0f);
            if (a == b)
                return luaL_error(L, "a joint needs two different bodies");
            if (axisX == 0.0f && axisY == 0.0f)
                return luaL_error(L, "the suspension axis cannot be (0, 0)");
            return PushJoint(L, WorldOf(L).createWheelJoint(a, b, {x, y}, {axisX, axisY}));
        }

        int PhysicsCreateWeldJoint(lua_State *L)
        {
            AccelEngine::RigidBody *a = BodyOf(L, 1);
            AccelEngine::RigidBody *b = BodyOf(L, 2);
            if (a == b)
                return luaL_error(L, "a joint needs two different bodies");
            return PushJoint(L, WorldOf(L).createWeldJoint(a, b));
        }

        const luaL_Reg PhysicsFunctions[] = {
            {"SetGravity", PhysicsSetGravity},
            {"GetGravity", PhysicsGetGravity},
            {"SetSubsteps", PhysicsSetSubsteps},
            {"GetSubsteps", PhysicsGetSubsteps},
            {"SetIterations", PhysicsSetIterations},
            {"SetSleepEnabled", PhysicsSetSleepEnabled},
            {"WakeAll", PhysicsWakeAll},
            {"GetBodyCount", PhysicsGetBodyCount},
            {"GetAwakeCount", PhysicsGetAwakeCount},
            {"Raycast", PhysicsRaycast},
            {"QueryPoint", PhysicsQueryPoint},
            {"QueryBox", PhysicsQueryBox},
            {"QueryCircle", PhysicsQueryCircle},
            {"CreateDistanceJoint", PhysicsCreateDistanceJoint},
            {"CreateRevoluteJoint", PhysicsCreateRevoluteJoint},
            {"CreateWheelJoint", PhysicsCreateWheelJoint},
            {"CreateWeldJoint", PhysicsCreateWeldJoint},
            {nullptr, nullptr}};

        AccelEngine::Joint *CheckJoint(lua_State *L)
        {
            JointHandle *handle = static_cast<JointHandle *>(luaL_checkudata(L, 1, JointMeta));
            AccelEngine::Joint *joint = handle->Joint;
            bool live = false;
            if (joint)
            {
                const auto &joints = WorldOf(L).getJoints();
                live = std::find(joints.begin(), joints.end(), joint) != joints.end();
            }
            if (!live)
            {
                handle->Joint = nullptr;
                luaL_error(L, "joint no longer exists");
                return nullptr;
            }
            return joint;
        }

        template <typename T>
        T *CheckJointKind(lua_State *L, const char *kind)
        {
            AccelEngine::Joint *joint = CheckJoint(L);
            T *typed = dynamic_cast<T *>(joint);
            if (!typed)
                luaL_error(L, "this method only works on %s joints", kind);
            return typed;
        }

        int JointIsValid(lua_State *L)
        {
            JointHandle *handle = static_cast<JointHandle *>(luaL_checkudata(L, 1, JointMeta));
            bool live = false;
            if (handle->Joint)
            {
                const auto &joints = WorldOf(L).getJoints();
                live = std::find(joints.begin(), joints.end(), handle->Joint) != joints.end();
            }
            lua_pushboolean(L, live);
            return 1;
        }

        int JointDestroy(lua_State *L)
        {
            AccelEngine::Joint *joint = CheckJoint(L);
            WorldOf(L).removeJoint(joint);
            static_cast<JointHandle *>(lua_touserdata(L, 1))->Joint = nullptr;
            return 0;
        }

        int JointGetType(lua_State *L)
        {
            AccelEngine::Joint *joint = CheckJoint(L);
            const char *type = dynamic_cast<AccelEngine::DistanceJoint *>(joint)   ? "distance"
                               : dynamic_cast<AccelEngine::RevoluteJoint *>(joint) ? "revolute"
                               : dynamic_cast<AccelEngine::WheelJoint *>(joint)    ? "wheel"
                               : dynamic_cast<AccelEngine::WeldJoint *>(joint)     ? "weld"
                                                                                    : "other";
            lua_pushstring(L, type);
            return 1;
        }

        int JointSetCollideConnected(lua_State *L)
        {
            AccelEngine::Joint *joint = CheckJoint(L);
            luaL_checkany(L, 2);
            joint->collideConnected = lua_toboolean(L, 2) != 0;
            return 0;
        }

        int JointSetLength(lua_State *L)
        {
            AccelEngine::DistanceJoint *joint = CheckJointKind<AccelEngine::DistanceJoint>(L, "distance");
            joint->restLength = std::max(Num(L, 2), 0.0f);
            joint->A->wakeUp();
            joint->B->wakeUp();
            return 0;
        }

        int JointGetLength(lua_State *L)
        {
            lua_pushnumber(L, CheckJointKind<AccelEngine::DistanceJoint>(L, "distance")->restLength);
            return 1;
        }

        int JointSetSoftness(lua_State *L)
        {
            AccelEngine::DistanceJoint *joint = CheckJointKind<AccelEngine::DistanceJoint>(L, "distance");
            joint->setCompliance(Num(L, 2));
            return 0;
        }

        int JointSetLimits(lua_State *L)
        {
            AccelEngine::Joint *joint = CheckJoint(L);
            const float lower = Num(L, 2), upper = Num(L, 3);
            if (auto *distance = dynamic_cast<AccelEngine::DistanceJoint *>(joint))
                distance->setLimits(std::min(lower, upper), std::max(lower, upper));
            else if (auto *revolute = dynamic_cast<AccelEngine::RevoluteJoint *>(joint))
                revolute->setLimits(std::clamp(glm::radians(std::min(lower, upper)), -3.1f, 3.1f), std::clamp(glm::radians(std::max(lower, upper)), -3.1f, 3.1f));
            else if (auto *wheel = dynamic_cast<AccelEngine::WheelJoint *>(joint))
                wheel->setLimits(std::min(lower, upper), std::max(lower, upper));
            else
                return luaL_error(L, "this joint has no limits");
            joint->A->wakeUp();
            joint->B->wakeUp();
            return 0;
        }

        int JointSetMotor(lua_State *L)
        {
            AccelEngine::Joint *joint = CheckJoint(L);
            const float speed = Num(L, 2), torque = Num(L, 3);
            if (auto *revolute = dynamic_cast<AccelEngine::RevoluteJoint *>(joint))
                revolute->setMotor(speed, torque);
            else if (auto *wheel = dynamic_cast<AccelEngine::WheelJoint *>(joint))
                wheel->setMotor(speed, torque);
            else
                return luaL_error(L, "only revolute and wheel joints have a motor");
            joint->A->wakeUp();
            joint->B->wakeUp();
            return 0;
        }

        int JointDisableMotor(lua_State *L)
        {
            AccelEngine::Joint *joint = CheckJoint(L);
            if (auto *revolute = dynamic_cast<AccelEngine::RevoluteJoint *>(joint))
                revolute->disableMotor();
            else if (auto *wheel = dynamic_cast<AccelEngine::WheelJoint *>(joint))
                wheel->disableMotor();
            else
                return luaL_error(L, "only revolute and wheel joints have a motor");
            return 0;
        }

        int JointSetSpring(lua_State *L)
        {
            AccelEngine::WheelJoint *joint = CheckJointKind<AccelEngine::WheelJoint>(L, "wheel");
            joint->setSpring(Num(L, 2), NumOpt(L, 3, joint->springDamping));
            return 0;
        }

        int JointGetAngle(lua_State *L)
        {
            lua_pushnumber(L, glm::degrees(CheckJointKind<AccelEngine::RevoluteJoint>(L, "revolute")->getAngle()));
            return 1;
        }

        int JointGetTranslation(lua_State *L)
        {
            lua_pushnumber(L, CheckJointKind<AccelEngine::WheelJoint>(L, "wheel")->getTranslation());
            return 1;
        }

        const luaL_Reg JointMethods[] = {
            {"IsValid", JointIsValid},
            {"Destroy", JointDestroy},
            {"GetType", JointGetType},
            {"SetCollideConnected", JointSetCollideConnected},
            {"SetLength", JointSetLength},
            {"GetLength", JointGetLength},
            {"SetSoftness", JointSetSoftness},
            {"SetLimits", JointSetLimits},
            {"SetMotor", JointSetMotor},
            {"DisableMotor", JointDisableMotor},
            {"SetSpring", JointSetSpring},
            {"GetAngle", JointGetAngle},
            {"GetTranslation", JointGetTranslation},
            {nullptr, nullptr}};
    }

    void RegisterPhysics(lua_State *L)
    {
        luaL_newlib(L, PhysicsFunctions);
        lua_setglobal(L, "Physics");

        luaL_newmetatable(L, JointMeta);
        luaL_newlib(L, JointMethods);
        lua_setfield(L, -2, "__index");
        lua_pop(L, 1);
    }
}
