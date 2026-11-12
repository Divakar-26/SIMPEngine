#include "LuaModules.h"

#include "LuaBindings.h"
#include "LuaCallback.h"
#include "LuaHelpers.h"

#include <Engine/Animation/Curves.h>
#include <Engine/Animation/Tween.h>
#include <Engine/Math/EntityUtility.h>
#include <Engine/Vector/VectorShape.h>

#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <limits>
#include <new>
#include <string>
#include <vector>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *TweenMeta = "SIMP.Tween";
        constexpr const char *CurveMeta = "SIMP.Curve";
        constexpr float Keep = std::numeric_limits<float>::quiet_NaN();

        struct TweenRef
        {
            uint64_t Serial;
            uint32_t Id;
        };

        struct Error
        {
            char Text[192] = "";
            bool Set() const { return Text[0] != '\0'; }
        };

        TweenRef *TestTween(lua_State *L, int index)
        {
            return static_cast<TweenRef *>(luaL_testudata(L, index, TweenMeta));
        }

        TweenNode *NodeOf(lua_State *L, int index, TweenSystem **systemOut = nullptr)
        {
            TweenRef *ref = static_cast<TweenRef *>(luaL_checkudata(L, index, TweenMeta));
            TweenSystem *system = TweenSystem::FromSerial(ref->Serial);
            if (systemOut)
                *systemOut = system;
            return system ? system->Get(ref->Id) : nullptr;
        }

        Entity *TestEntity(lua_State *L, int index)
        {
            Entity *entity = static_cast<Entity *>(luaL_testudata(L, index, EntityMeta));
            return entity && static_cast<bool>(*entity) ? entity : nullptr;
        }

        void PushCurve(lua_State *L, const Curve &curve)
        {
            void *memory = lua_newuserdatauv(L, sizeof(Curve), 0);
            new (memory) Curve(curve);
            luaL_setmetatable(L, CurveMeta);
        }

        bool ReadNumberArray(lua_State *L, int index, float *out, int count)
        {
            for (int i = 0; i < count; ++i)
            {
                lua_rawgeti(L, index, i + 1);
                if (lua_isnumber(L, -1))
                    out[i] = static_cast<float>(lua_tonumber(L, -1));
                lua_pop(L, 1);
            }
            return true;
        }

        bool ParseProps(lua_State *L, int index, std::vector<TweenTrack> &tracks, Error &error)
        {
            if (lua_type(L, index) != LUA_TTABLE)
            {
                std::snprintf(error.Text, sizeof(error.Text), "expected a table of properties");
                return false;
            }
            index = lua_absindex(L, index);
            lua_pushnil(L);
            while (lua_next(L, index) != 0)
            {
                if (lua_type(L, -2) != LUA_TSTRING)
                {
                    lua_pop(L, 2);
                    std::snprintf(error.Text, sizeof(error.Text), "property names must be strings");
                    return false;
                }
                const char *name = lua_tostring(L, -2);
                TweenTrack track;
                bool known = false;
                {
                    TweenProperty property;
                    int channels = 1;
                    known = TweenSystem::PropertyFromName(name, property, channels);
                    if (!known && name[0] == 'u' && std::isupper(static_cast<unsigned char>(name[1])))
                    {
                        known = true;
                        property = TweenProperty::ShaderUniform;
                        track.Uniform = name;
                        channels = lua_type(L, -1) == LUA_TTABLE ? static_cast<int>(std::clamp<lua_Unsigned>(lua_rawlen(L, -1), 1, 4)) : 1;
                    }
                    track.Property = property;
                    track.Channels = channels;
                }
                if (!known)
                {
                    std::snprintf(error.Text, sizeof(error.Text), "unknown tween property '%s'", name);
                    lua_pop(L, 2);
                    return false;
                }
                for (int c = 0; c < 4; ++c)
                    track.To[c] = Keep;

                const int type = lua_type(L, -1);
                if (type == LUA_TNUMBER)
                {
                    if (track.Channels == 4 || track.Property == TweenProperty::Position)
                    {
                        std::snprintf(error.Text, sizeof(error.Text), "'%s' needs a table value", name);
                        lua_pop(L, 2);
                        return false;
                    }
                    const float v = static_cast<float>(lua_tonumber(L, -1));
                    for (int c = 0; c < track.Channels; ++c)
                        track.To[c] = v;
                }
                else if (type == LUA_TSTRING)
                {
                    const char *text = lua_tostring(L, -1);
                    float sign = 0.0f;
                    if (std::strncmp(text, "+=", 2) == 0)
                        sign = 1.0f;
                    else if (std::strncmp(text, "-=", 2) == 0)
                        sign = -1.0f;
                    char *end = nullptr;
                    const float amount = sign != 0.0f ? std::strtof(text + 2, &end) : 0.0f;
                    if (sign == 0.0f || end == text + 2 || track.Channels == 4)
                    {
                        std::snprintf(error.Text, sizeof(error.Text), "'%s': relative values look like \"+=100\" or \"-=50\"", name);
                        lua_pop(L, 2);
                        return false;
                    }
                    track.Relative = true;
                    for (int c = 0; c < track.Channels; ++c)
                        track.To[c] = sign * amount;
                }
                else if (type == LUA_TTABLE)
                {
                    ReadNumberArray(L, lua_gettop(L), track.To, track.Channels);
                }
                else
                {
                    std::snprintf(error.Text, sizeof(error.Text), "'%s' has an unsupported value", name);
                    lua_pop(L, 2);
                    return false;
                }
                tracks.push_back(track);
                lua_pop(L, 1);
            }
            return true;
        }

        float ReadSpringParam(lua_State *L, int index, const char *key, float fallback)
        {
            if (lua_type(L, index) != LUA_TTABLE)
                return fallback;
            lua_getfield(L, index, key);
            const float v = lua_isnumber(L, -1) ? static_cast<float>(lua_tonumber(L, -1)) : fallback;
            lua_pop(L, 1);
            return v;
        }

        enum class Mode
        {
            To,
            From,
            FromTo
        };

        int Create(lua_State *L, Mode mode)
        {
            Entity *entity = TestEntity(L, 1);
            if (!entity)
                return luaL_error(L, "Tween: argument 1 must be a live entity");
            const int toIndex = mode == Mode::FromTo ? 3 : 2;
            const int durationIndex = toIndex + 1;
            const float duration = static_cast<float>(luaL_checknumber(L, durationIndex));
            const int curveIndex = durationIndex + 1;
            const int optsIndex = durationIndex + 2;

            TweenSystem &system = entity->GetScene()->GetTweens();
            const entt::entity target = entity->GetHandle();
            Error error;
            uint32_t id = 0;
            {
                Curve curve;
                std::vector<TweenTrack> to;
                std::vector<TweenTrack> from;
                if (!ReadCurve(L, curveIndex, curve))
                    std::snprintf(error.Text, sizeof(error.Text), "unknown curve (see Curves.Names())");
                else if (ParseProps(L, toIndex, to, error) && (mode != Mode::FromTo || ParseProps(L, 2, from, error)))
                {
                    if (mode == Mode::From)
                    {
                        for (TweenTrack &track : to)
                        {
                            std::copy(track.To, track.To + 4, track.From);
                            track.ExplicitFrom = true;
                            track.RelativeFrom = track.Relative;
                            track.Relative = false;
                            for (float &v : track.To)
                                v = Keep;
                        }
                    }
                    else if (mode == Mode::FromTo)
                    {
                        for (TweenTrack &track : to)
                            for (const TweenTrack &f : from)
                                if (f.Property == track.Property)
                                {
                                    std::copy(f.To, f.To + 4, track.From);
                                    track.ExplicitFrom = true;
                                    track.RelativeFrom = f.Relative;
                                }
                    }
                    id = system.To(target, std::move(to), duration, curve);
                    if (mode != Mode::To)
                        system.ImmediateRender(id);
                }
            }
            if (error.Set())
                return luaL_error(L, "Tween: %s", error.Text);
            ApplyTweenOptions(L, optsIndex, system, id);
            return PushTween(L, system.Serial(), id);
        }

        int TweenTo(lua_State *L) { return Create(L, Mode::To); }
        int TweenFrom(lua_State *L) { return Create(L, Mode::From); }
        int TweenFromTo(lua_State *L) { return Create(L, Mode::FromTo); }

        int TweenValue(lua_State *L)
        {
            const float from = static_cast<float>(luaL_checknumber(L, 1));
            const float to = static_cast<float>(luaL_checknumber(L, 2));
            const float duration = static_cast<float>(luaL_checknumber(L, 3));
            luaL_checktype(L, 5, LUA_TFUNCTION);
            Scene *scene = RequireScene(L);
            TweenSystem &system = scene->GetTweens();
            uint32_t id = 0;
            bool badCurve = false;
            {
                Curve curve;
                if (!ReadCurve(L, 4, curve))
                    badCurve = true;
                else
                {
                    std::shared_ptr<LuaCallback> cb = LuaCallback::FromStack(L, 5);
                    id = system.Value(from, to, duration, curve, [cb, scene](float v)
                                      { cb->Call(scene, v); });
                }
            }
            if (badCurve)
                return luaL_error(L, "Tween.Value: unknown curve");
            ApplyTweenOptions(L, 6, system, id);
            return PushTween(L, system.Serial(), id);
        }

        int TweenAlong(lua_State *L)
        {
            Entity *entity = TestEntity(L, 1);
            if (!entity)
                return luaL_error(L, "Tween.Along: argument 1 must be a live entity");
            VectorPath *path = TestPath(L, 2);
            Entity *source = path ? nullptr : TestEntity(L, 2);
            if (!path && (!source || !source->HasComponent<VectorShapeComponent>()))
                return luaL_error(L, "Tween.Along: argument 2 must be a Path or an entity with a Vector shape");
            const float duration = static_cast<float>(luaL_checknumber(L, 3));

            bool orient = false;
            bool relative = false;
            if (lua_type(L, 5) == LUA_TTABLE)
            {
                lua_getfield(L, 5, "orient");
                orient = lua_toboolean(L, -1) != 0;
                lua_getfield(L, 5, "relative");
                relative = lua_toboolean(L, -1) != 0;
                lua_pop(L, 2);
            }

            TweenSystem &system = entity->GetScene()->GetTweens();
            uint32_t id = 0;
            bool badCurve = false;
            bool empty = false;
            {
                Curve curve;
                std::vector<FlatContour> outline;
                if (path)
                    path->Flatten(0.25f, outline);
                else
                {
                    const auto &shape = source->GetComponent<VectorShapeComponent>();
                    VectorShapes::Outline(shape, 0.25f, outline);
                    const WorldTransform2D world = DecomposeWorld(source->GetComponent<TransformComponent>());
                    const float r = glm::radians(world.Rotation);
                    const float c = std::cos(r);
                    const float s = std::sin(r);
                    for (FlatContour &contour : outline)
                        for (glm::vec2 &p : contour.Points)
                        {
                            const glm::vec2 q = p * world.Scale;
                            p = world.Position + glm::vec2(q.x * c - q.y * s, q.x * s + q.y * c);
                        }
                }
                if (!ReadCurve(L, 4, curve))
                    badCurve = true;
                else if (outline.empty() || outline.front().Points.empty())
                    empty = true;
                else
                    id = system.Motion(entity->GetHandle(), outline.front().Points, outline.front().Closed, duration, curve, orient, relative);
            }
            if (badCurve)
                return luaL_error(L, "Tween.Along: unknown curve");
            if (empty)
                return luaL_error(L, "Tween.Along: the path is empty");
            ApplyTweenOptions(L, 5, system, id);
            return PushTween(L, system.Serial(), id);
        }

        int TweenSpring(lua_State *L)
        {
            Entity *entity = TestEntity(L, 1);
            if (!entity)
                return luaL_error(L, "Tween.Spring: argument 1 must be a live entity");
            TweenSystem &system = entity->GetScene()->GetTweens();
            const entt::entity target = entity->GetHandle();
            SpringParams params;
            params.Stiffness = ReadSpringParam(L, 3, "stiffness", params.Stiffness);
            params.Damping = ReadSpringParam(L, 3, "damping", params.Damping);
            params.Mass = ReadSpringParam(L, 3, "mass", params.Mass);
            params.RestThreshold = ReadSpringParam(L, 3, "threshold", params.RestThreshold);
            const bool customParams = lua_type(L, 3) == LUA_TTABLE;

            Error error;
            uint32_t id = 0;
            {
                std::vector<TweenTrack> tracks;
                if (ParseProps(L, 2, tracks, error))
                {
                    for (TweenTrack &track : tracks)
                    {
                        float current[4] = {0.0f, 0.0f, 0.0f, 0.0f};
                        system.Read(target, track.Property, current);
                        for (int c = 0; c < 4; ++c)
                        {
                            if (track.Relative && !std::isnan(track.To[c]))
                                track.To[c] += current[c];
                            else if (std::isnan(track.To[c]))
                                track.To[c] = current[c];
                        }
                        track.Relative = false;
                    }
                    if (TweenNode *existing = system.FindSpring(target))
                    {
                        id = existing->Id;
                        system.RetargetSpring(id, tracks, customParams ? &params : nullptr);
                    }
                    else
                        id = system.Spring(target, std::move(tracks), params);
                }
            }
            if (error.Set())
                return luaL_error(L, "Tween.Spring: %s", error.Text);
            if (lua_type(L, 3) == LUA_TTABLE)
            {
                if (TweenNode *node = system.Get(id))
                {
                    Scene *scene = &system.GetScene();
                    lua_getfield(L, 3, "onComplete");
                    std::shared_ptr<LuaCallback> cb = LuaCallback::FromStack(L, -1);
                    lua_pop(L, 1);
                    if (cb)
                        node->OnComplete = [cb, scene]()
                        { cb->Call(scene); };
                }
            }
            return PushTween(L, system.Serial(), id);
        }

        int MakeGroup(lua_State *L, bool sequence)
        {
            luaL_checktype(L, 1, LUA_TTABLE);
            const lua_Integer count = luaL_len(L, 1);
            TweenSystem *system = nullptr;
            for (lua_Integer i = 1; i <= count && !system; ++i)
            {
                lua_rawgeti(L, 1, i);
                if (TweenRef *ref = TestTween(L, -1))
                    system = TweenSystem::FromSerial(ref->Serial);
                lua_pop(L, 1);
            }
            if (!system)
                system = &RequireScene(L)->GetTweens();
            Scene *scene = &system->GetScene();

            Error error;
            uint32_t id = 0;
            {
                std::vector<uint32_t> children;
                for (lua_Integer i = 1; i <= count && !error.Set(); ++i)
                {
                    lua_rawgeti(L, 1, i);
                    if (TweenRef *ref = TestTween(L, -1))
                    {
                        if (ref->Serial != system->Serial())
                            std::snprintf(error.Text, sizeof(error.Text), "all tweens in a group must belong to the same scene");
                        else
                            children.push_back(ref->Id);
                    }
                    else if (lua_isnumber(L, -1))
                        children.push_back(system->Delay(static_cast<float>(lua_tonumber(L, -1))));
                    else if (lua_type(L, -1) == LUA_TFUNCTION)
                    {
                        std::shared_ptr<LuaCallback> cb = LuaCallback::FromStack(L, -1);
                        children.push_back(system->Call([cb, scene]()
                                                        { cb->Call(scene); }));
                    }
                    else
                        std::snprintf(error.Text, sizeof(error.Text), "item %d must be a tween, a number (delay) or a function", static_cast<int>(i));
                    lua_pop(L, 1);
                }
                if (!error.Set())
                {
                    std::string message;
                    id = system->Group(sequence, children, &message);
                    if (id == 0)
                        std::snprintf(error.Text, sizeof(error.Text), "%s", message.c_str());
                }
            }
            if (error.Set())
                return luaL_error(L, "Tween.%s: %s", sequence ? "Sequence" : "Parallel", error.Text);
            ApplyTweenOptions(L, 2, *system, id);
            return PushTween(L, system->Serial(), id);
        }

        int TweenSequence(lua_State *L) { return MakeGroup(L, true); }
        int TweenParallel(lua_State *L) { return MakeGroup(L, false); }

        int TweenStagger(lua_State *L)
        {
            luaL_checktype(L, 1, LUA_TTABLE);
            luaL_checktype(L, 2, LUA_TTABLE);
            const float duration = static_cast<float>(luaL_checknumber(L, 3));
            const float each = static_cast<float>(luaL_optnumber(L, 5, 0.1));
            const lua_Integer count = luaL_len(L, 1);
            if (count == 0)
                return luaL_error(L, "Tween.Stagger: no entities");

            TweenSystem *system = nullptr;
            Error error;
            uint32_t id = 0;
            {
                Curve curve;
                std::vector<TweenTrack> tracks;
                std::vector<uint32_t> children;
                if (!ReadCurve(L, 4, curve))
                    std::snprintf(error.Text, sizeof(error.Text), "unknown curve");
                else if (ParseProps(L, 2, tracks, error))
                {
                    for (lua_Integer i = 1; i <= count && !error.Set(); ++i)
                    {
                        lua_rawgeti(L, 1, i);
                        Entity *entity = TestEntity(L, -1);
                        if (!entity)
                            std::snprintf(error.Text, sizeof(error.Text), "item %d is not a live entity", static_cast<int>(i));
                        else
                        {
                            if (!system)
                                system = &entity->GetScene()->GetTweens();
                            const uint32_t child = system->To(entity->GetHandle(), tracks, duration, curve);
                            system->Get(child)->Delay = each * static_cast<float>(i - 1);
                            children.push_back(child);
                        }
                        lua_pop(L, 1);
                    }
                    if (!error.Set())
                    {
                        std::string message;
                        id = system->Group(false, children, &message);
                        if (id == 0)
                            std::snprintf(error.Text, sizeof(error.Text), "%s", message.c_str());
                    }
                }
            }
            if (error.Set())
                return luaL_error(L, "Tween.Stagger: %s", error.Text);
            ApplyTweenOptions(L, 6, *system, id);
            return PushTween(L, system->Serial(), id);
        }

        int TweenDelayNode(lua_State *L)
        {
            const float seconds = static_cast<float>(luaL_checknumber(L, 1));
            TweenSystem &system = RequireScene(L)->GetTweens();
            return PushTween(L, system.Serial(), system.Delay(seconds));
        }

        int TweenCallNode(lua_State *L)
        {
            luaL_checktype(L, 1, LUA_TFUNCTION);
            Scene *scene = RequireScene(L);
            TweenSystem &system = scene->GetTweens();
            uint32_t id = 0;
            {
                std::shared_ptr<LuaCallback> cb = LuaCallback::FromStack(L, 1);
                id = system.Call([cb, scene]()
                                 { cb->Call(scene); });
            }
            return PushTween(L, system.Serial(), id);
        }

        int TweenKillTarget(lua_State *L)
        {
            Entity *entity = TestEntity(L, 1);
            if (!entity)
                return 0;
            int property = -1;
            std::string uniformName;
            if (lua_type(L, 2) == LUA_TSTRING)
            {
                TweenProperty p;
                int channels = 1;
                const char *text = lua_tostring(L, 2);
                if (TweenSystem::PropertyFromName(text, p, channels))
                    property = static_cast<int>(p);
                else if (text[0] == 'u' && std::isupper(static_cast<unsigned char>(text[1])))
                {
                    property = static_cast<int>(TweenProperty::ShaderUniform);
                    uniformName = text;
                }
            }
            entity->GetScene()->GetTweens().KillTarget(entity->GetHandle(), property, uniformName);
            return 0;
        }

        int TweenKillAll(lua_State *L)
        {
            RequireScene(L)->GetTweens().Clear();
            return 0;
        }

        int TweenIsTweening(lua_State *L)
        {
            Entity *entity = TestEntity(L, 1);
            lua_pushboolean(L, entity && entity->GetScene()->GetTweens().IsTweening(entity->GetHandle()));
            return 1;
        }

        int TweenCount(lua_State *L)
        {
            lua_pushinteger(L, static_cast<lua_Integer>(RequireScene(L)->GetTweens().Count()));
            return 1;
        }

        int TweenProperties(lua_State *L)
        {
            lua_newtable(L);
            for (int i = 0; i < static_cast<int>(TweenProperty::Count); ++i)
            {
                lua_pushstring(L, TweenSystem::PropertyName(static_cast<TweenProperty>(i)));
                lua_rawseti(L, -2, i + 1);
            }
            return 1;
        }

        int SelfReturn(lua_State *L)
        {
            lua_pushvalue(L, 1);
            return 1;
        }

        TweenNode *ConfigurableNode(lua_State *L)
        {
            TweenNode *node = NodeOf(L, 1);
            if (node && node->Owned)
                luaL_error(L, "configure a tween before adding it to a Sequence/Parallel");
            return node;
        }

        int HandleDelay(lua_State *L)
        {
            const float seconds = static_cast<float>(luaL_checknumber(L, 2));
            if (TweenNode *node = ConfigurableNode(L))
                node->Delay = std::max(seconds, 0.0f);
            return SelfReturn(L);
        }

        int HandleRepeat(lua_State *L)
        {
            const int count = static_cast<int>(luaL_optinteger(L, 2, -1));
            if (TweenNode *node = ConfigurableNode(L))
                node->Repeat = count < 0 ? -1 : count;
            return SelfReturn(L);
        }

        int HandleYoyo(lua_State *L)
        {
            const bool yoyo = lua_isnone(L, 2) ? true : lua_toboolean(L, 2) != 0;
            if (TweenNode *node = ConfigurableNode(L))
                node->Yoyo = yoyo;
            return SelfReturn(L);
        }

        int HandleSpeed(lua_State *L)
        {
            const float speed = static_cast<float>(luaL_checknumber(L, 2));
            if (TweenNode *node = NodeOf(L, 1))
                node->Speed = std::max(speed, 0.0f);
            return SelfReturn(L);
        }

        int HandleCurve(lua_State *L)
        {
            TweenNode *node = NodeOf(L, 1);
            bool ok = true;
            {
                Curve curve;
                ok = ReadCurve(L, 2, curve);
                if (ok && node)
                    node->Ease = curve;
            }
            if (!ok)
                return luaL_error(L, "unknown curve");
            return SelfReturn(L);
        }

        template <int Which>
        int HandleCallback(lua_State *L)
        {
            luaL_checktype(L, 2, LUA_TFUNCTION);
            TweenSystem *system = nullptr;
            TweenNode *node = NodeOf(L, 1, &system);
            if (node)
            {
                Scene *scene = &system->GetScene();
                std::shared_ptr<LuaCallback> cb = LuaCallback::FromStack(L, 2);
                if constexpr (Which == 0)
                    node->OnStart = [cb, scene]()
                    { cb->Call(scene); };
                else if constexpr (Which == 1)
                    node->OnUpdate = [cb, scene](float v)
                    { cb->Call(scene, v); };
                else if constexpr (Which == 2)
                    node->OnComplete = [cb, scene]()
                    { cb->Call(scene); };
                else
                    node->OnRepeat = [cb, scene]()
                    { cb->Call(scene); };
            }
            return SelfReturn(L);
        }

        template <void (TweenSystem::*Action)(uint32_t)>
        int HandleAction(lua_State *L)
        {
            TweenRef *ref = static_cast<TweenRef *>(luaL_checkudata(L, 1, TweenMeta));
            if (TweenSystem *system = TweenSystem::FromSerial(ref->Serial))
                (system->*Action)(ref->Id);
            return SelfReturn(L);
        }

        int HandleSeek(lua_State *L)
        {
            TweenRef *ref = static_cast<TweenRef *>(luaL_checkudata(L, 1, TweenMeta));
            const float progress = static_cast<float>(luaL_checknumber(L, 2));
            if (TweenSystem *system = TweenSystem::FromSerial(ref->Serial))
                system->Seek(ref->Id, progress);
            return SelfReturn(L);
        }

        int HandleProgress(lua_State *L)
        {
            TweenRef *ref = static_cast<TweenRef *>(luaL_checkudata(L, 1, TweenMeta));
            TweenSystem *system = TweenSystem::FromSerial(ref->Serial);
            lua_pushnumber(L, system ? system->Progress(ref->Id) : 1.0);
            return 1;
        }

        int HandleIsPlaying(lua_State *L)
        {
            TweenRef *ref = static_cast<TweenRef *>(luaL_checkudata(L, 1, TweenMeta));
            TweenSystem *system = TweenSystem::FromSerial(ref->Serial);
            lua_pushboolean(L, system && system->IsRunning(ref->Id));
            return 1;
        }

        int HandleIsFinished(lua_State *L)
        {
            TweenNode *node = NodeOf(L, 1);
            lua_pushboolean(L, !node || node->Dead || node->Finished);
            return 1;
        }

        int HandleDuration(lua_State *L)
        {
            TweenNode *node = NodeOf(L, 1);
            lua_pushnumber(L, node ? node->TotalLength() : 0.0);
            return 1;
        }

        int HandleGc(lua_State *L)
        {
            TweenRef *ref = static_cast<TweenRef *>(luaL_checkudata(L, 1, TweenMeta));
            if (TweenSystem *system = TweenSystem::FromSerial(ref->Serial))
                system->Release(ref->Id);
            return 0;
        }

        int HandleToString(lua_State *L)
        {
            TweenRef *ref = static_cast<TweenRef *>(luaL_checkudata(L, 1, TweenMeta));
            lua_pushfstring(L, "Tween(%d)", static_cast<int>(ref->Id));
            return 1;
        }

        const luaL_Reg TweenFunctions[] = {
            {"To", TweenTo},
            {"From", TweenFrom},
            {"FromTo", TweenFromTo},
            {"Value", TweenValue},
            {"Along", TweenAlong},
            {"Spring", TweenSpring},
            {"Sequence", TweenSequence},
            {"Parallel", TweenParallel},
            {"Stagger", TweenStagger},
            {"Delay", TweenDelayNode},
            {"Call", TweenCallNode},
            {"Kill", TweenKillTarget},
            {"KillAll", TweenKillAll},
            {"IsTweening", TweenIsTweening},
            {"Count", TweenCount},
            {"Properties", TweenProperties},
            {nullptr, nullptr}};

        const luaL_Reg TweenMethods[] = {
            {"Delay", HandleDelay},
            {"Repeat", HandleRepeat},
            {"Yoyo", HandleYoyo},
            {"Speed", HandleSpeed},
            {"Curve", HandleCurve},
            {"OnStart", HandleCallback<0>},
            {"OnUpdate", HandleCallback<1>},
            {"OnComplete", HandleCallback<2>},
            {"Then", HandleCallback<2>},
            {"OnRepeat", HandleCallback<3>},
            {"Play", HandleAction<&TweenSystem::Play>},
            {"Resume", HandleAction<&TweenSystem::Play>},
            {"Pause", HandleAction<&TweenSystem::Pause>},
            {"Restart", HandleAction<&TweenSystem::Restart>},
            {"Reverse", HandleAction<&TweenSystem::Reverse>},
            {"Forward", HandleAction<&TweenSystem::Forward>},
            {"Kill", HandleAction<&TweenSystem::Kill>},
            {"Seek", HandleSeek},
            {"Progress", HandleProgress},
            {"IsPlaying", HandleIsPlaying},
            {"IsFinished", HandleIsFinished},
            {"Duration", HandleDuration},
            {nullptr, nullptr}};

        Curve *CheckCurve(lua_State *L, int index)
        {
            return static_cast<Curve *>(luaL_checkudata(L, index, CurveMeta));
        }

        int CurveGc(lua_State *L)
        {
            CheckCurve(L, 1)->~Curve();
            return 0;
        }

        int CurveTransform(lua_State *L)
        {
            const Curve *curve = CheckCurve(L, 1);
            lua_pushnumber(L, curve->Evaluate(static_cast<float>(luaL_checknumber(L, 2))));
            return 1;
        }

        int CurveFlippedMethod(lua_State *L)
        {
            PushCurve(L, Curve::Flipped(*CheckCurve(L, 1)));
            return 1;
        }

        int CurveToString(lua_State *L)
        {
            lua_pushfstring(L, "Curve(%s)", CheckCurve(L, 1)->GetName().c_str());
            return 1;
        }

        int CurvesCubic(lua_State *L)
        {
            const float a = static_cast<float>(luaL_checknumber(L, 1));
            const float b = static_cast<float>(luaL_checknumber(L, 2));
            const float c = static_cast<float>(luaL_checknumber(L, 3));
            const float d = static_cast<float>(luaL_checknumber(L, 4));
            PushCurve(L, Curve::Cubic(a, b, c, d));
            return 1;
        }

        int CurvesInterval(lua_State *L)
        {
            const float begin = static_cast<float>(luaL_checknumber(L, 1));
            const float end = static_cast<float>(luaL_checknumber(L, 2));
            bool ok = true;
            {
                Curve inner = Curve::Linear();
                ok = lua_isnoneornil(L, 3) || ReadCurve(L, 3, inner);
                if (ok)
                    PushCurve(L, Curve::Interval(begin, end, inner));
            }
            if (!ok)
                return luaL_error(L, "Curves.Interval: unknown curve");
            return 1;
        }

        int CurvesSteps(lua_State *L)
        {
            PushCurve(L, Curve::Steps(static_cast<int>(luaL_checkinteger(L, 1))));
            return 1;
        }

        int CurvesSawTooth(lua_State *L)
        {
            PushCurve(L, Curve::SawTooth(static_cast<int>(luaL_checkinteger(L, 1))));
            return 1;
        }

        int CurvesThreshold(lua_State *L)
        {
            PushCurve(L, Curve::Threshold(static_cast<float>(luaL_checknumber(L, 1))));
            return 1;
        }

        int CurvesElastic(lua_State *L)
        {
            const char *kind = luaL_optstring(L, 1, "out");
            const float period = static_cast<float>(luaL_optnumber(L, 2, 0.4));
            CurveKind k = CurveKind::ElasticOut;
            if (std::strcmp(kind, "in") == 0)
                k = CurveKind::ElasticIn;
            else if (std::strcmp(kind, "inOut") == 0)
                k = CurveKind::ElasticInOut;
            PushCurve(L, Curve::Elastic(k, period));
            return 1;
        }

        int CurvesSpring(lua_State *L)
        {
            const float damping = static_cast<float>(luaL_optnumber(L, 1, 0.35));
            const float frequency = static_cast<float>(luaL_optnumber(L, 2, 3.0));
            PushCurve(L, Curve::Spring(damping, frequency));
            return 1;
        }

        int CurvesSine(lua_State *L)
        {
            PushCurve(L, Curve::Sine(static_cast<float>(luaL_optnumber(L, 1, 1.0))));
            return 1;
        }

        int CurvesWrap(lua_State *L, bool flipped)
        {
            bool ok = true;
            {
                Curve inner;
                ok = ReadCurve(L, 1, inner);
                if (ok)
                    PushCurve(L, flipped ? Curve::Flipped(inner) : Curve::Reversed(inner));
            }
            if (!ok)
                return luaL_error(L, "unknown curve");
            return 1;
        }

        int CurvesFlipped(lua_State *L) { return CurvesWrap(L, true); }
        int CurvesReversed(lua_State *L) { return CurvesWrap(L, false); }

        int CurvesEvaluate(lua_State *L)
        {
            const float t = static_cast<float>(luaL_checknumber(L, 2));
            float value = 0.0f;
            bool ok = true;
            {
                Curve curve;
                ok = ReadCurve(L, 1, curve);
                value = curve.Evaluate(t);
            }
            if (!ok)
                return luaL_error(L, "Curves.Evaluate: unknown curve");
            lua_pushnumber(L, value);
            return 1;
        }

        int CurvesNames(lua_State *L)
        {
            const auto &names = Curve::Names();
            lua_createtable(L, static_cast<int>(names.size()), 0);
            for (size_t i = 0; i < names.size(); ++i)
            {
                lua_pushstring(L, names[i].c_str());
                lua_rawseti(L, -2, static_cast<lua_Integer>(i + 1));
            }
            return 1;
        }

        const luaL_Reg CurveFunctions[] = {
            {"Cubic", CurvesCubic},
            {"Interval", CurvesInterval},
            {"Steps", CurvesSteps},
            {"SawTooth", CurvesSawTooth},
            {"Threshold", CurvesThreshold},
            {"Elastic", CurvesElastic},
            {"Spring", CurvesSpring},
            {"Sine", CurvesSine},
            {"Flipped", CurvesFlipped},
            {"Reversed", CurvesReversed},
            {"Evaluate", CurvesEvaluate},
            {"Names", CurvesNames},
            {nullptr, nullptr}};

        const luaL_Reg CurveMethods[] = {
            {"Transform", CurveTransform},
            {"Flipped", CurveFlippedMethod},
            {nullptr, nullptr}};
    }

    void ApplyTweenOptions(lua_State *L, int index, TweenSystem &system, uint32_t id)
    {
        TweenNode *node = system.Get(id);
        if (!node || lua_type(L, index) != LUA_TTABLE)
            return;
        Scene *scene = &system.GetScene();
        index = lua_absindex(L, index);

        auto number = [&](const char *key, float &out)
        {
            lua_getfield(L, index, key);
            if (lua_isnumber(L, -1))
                out = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);
        };
        auto boolean = [&](const char *key, bool &out)
        {
            lua_getfield(L, index, key);
            if (!lua_isnil(L, -1))
                out = lua_toboolean(L, -1) != 0;
            lua_pop(L, 1);
        };
        auto callback = [&](const char *key)
        {
            lua_getfield(L, index, key);
            std::shared_ptr<LuaCallback> cb = LuaCallback::FromStack(L, -1);
            lua_pop(L, 1);
            return cb;
        };

        number("delay", node->Delay);
        float repeat = static_cast<float>(node->Repeat);
        number("repeat", repeat);
        lua_getfield(L, index, "loop");
        if (lua_isboolean(L, -1))
            repeat = lua_toboolean(L, -1) ? -1.0f : 0.0f;
        else if (lua_isnumber(L, -1))
            repeat = static_cast<float>(lua_tonumber(L, -1));
        lua_pop(L, 1);
        node->Repeat = repeat < 0.0f ? -1 : static_cast<int>(repeat);
        boolean("yoyo", node->Yoyo);
        number("speed", node->Speed);
        bool paused = false;
        boolean("paused", paused);
        if (paused)
            node->Playing = false;
        number("orientOffset", node->OrientOffset);

        if (auto cb = callback("onStart"))
            node->OnStart = [cb, scene]()
            { cb->Call(scene); };
        if (auto cb = callback("onUpdate"))
            node->OnUpdate = [cb, scene](float v)
            { cb->Call(scene, v); };
        if (auto cb = callback("onComplete"))
            node->OnComplete = [cb, scene]()
            { cb->Call(scene); };
        if (auto cb = callback("onRepeat"))
            node->OnRepeat = [cb, scene]()
            { cb->Call(scene); };
    }

    bool ReadCurve(lua_State *L, int index, Curve &out)
    {
        if (lua_isnoneornil(L, index))
        {
            Curve::FromName("easeInOut", out);
            return true;
        }
        if (lua_type(L, index) == LUA_TSTRING)
            return Curve::FromName(lua_tostring(L, index), out);
        if (Curve *curve = static_cast<Curve *>(luaL_testudata(L, index, CurveMeta)))
        {
            out = *curve;
            return true;
        }
        return false;
    }

    int PushTween(lua_State *L, uint64_t serial, uint32_t id)
    {
        TweenRef *ref = static_cast<TweenRef *>(lua_newuserdatauv(L, sizeof(TweenRef), 0));
        ref->Serial = serial;
        ref->Id = id;
        luaL_setmetatable(L, TweenMeta);
        if (TweenSystem *system = TweenSystem::FromSerial(serial))
            system->AddRef(id);
        return 1;
    }

    void RegisterTween(lua_State *L)
    {
        luaL_newmetatable(L, TweenMeta);
        luaL_newlib(L, TweenMethods);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, HandleGc);
        lua_setfield(L, -2, "__gc");
        lua_pushcfunction(L, HandleToString);
        lua_setfield(L, -2, "__tostring");
        lua_pop(L, 1);

        luaL_newmetatable(L, CurveMeta);
        luaL_newlib(L, CurveMethods);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, CurveGc);
        lua_setfield(L, -2, "__gc");
        lua_pushcfunction(L, CurveToString);
        lua_setfield(L, -2, "__tostring");
        lua_pop(L, 1);

        luaL_newlib(L, TweenFunctions);
        lua_setglobal(L, "Tween");

        luaL_newlib(L, CurveFunctions);
        for (const std::string &name : Curve::Names())
        {
            Curve curve;
            Curve::FromName(name, curve);
            PushCurve(L, curve);
            lua_setfield(L, -2, name.c_str());
        }
        lua_setglobal(L, "Curves");
    }
}
