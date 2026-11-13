#include "LuaBindings.h"
#include "LuaInclude.h"

#include "LuaHelpers.h"
#include "LuaScript.h"
#include "Components/LuaComponents.h"
#include "LuaModules.h"

#include <Engine/Application/Application.h>
#include <Engine/Audio/AudioEngine.h>
#include <Engine/Audio/Music.h>
#include <Engine/Core/GameVariables.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/Display.h>
#include <Engine/Input/Input.h>
#include <Engine/Input/InputActions.h>
#include <Engine/Input/SIMP_Keys.h>
#include <Engine/Math/EntityUtility.h>
#include <Engine/Particles/ParticleEmitter.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Prefab.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/ScriptableEntity.h>
#include <Engine/Scene/TilemapUtils.h>

#include <algorithm>
#include <cmath>
#include <new>
#include <string>
#include <string_view>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        Scene *s_ActiveScene = nullptr;

        struct KeyEntry
        {
            const char *Name;
            int Code;
        };

        constexpr KeyEntry Keys[] = {
        {"W", SIMPK_W},
        {"A", SIMPK_A},
        {"S", SIMPK_S},
        {"D", SIMPK_D},
        {"Q", SIMPK_Q},
        {"E", SIMPK_E},
        {"F", SIMPK_F},
        {"G", SIMPK_G},
        {"R", SIMPK_R},
        {"T", SIMPK_T},
        {"Y", SIMPK_Y},
        {"U", SIMPK_U},
        {"I", SIMPK_I},
        {"O", SIMPK_O},
        {"P", SIMPK_P},
        {"Z", SIMPK_Z},
        {"X", SIMPK_X},
        {"C", SIMPK_C},
        {"V", SIMPK_V},
        {"B", SIMPK_B},
        {"N", SIMPK_N},
        {"M", SIMPK_M},
        {"L", SIMPK_L},
        {"H", SIMPK_H},
        {"J", SIMPK_J},
        {"K", SIMPK_K},
        {"NUM_0", SIMPK_0},
        {"NUM_1", SIMPK_1},
        {"NUM_2", SIMPK_2},
        {"NUM_3", SIMPK_3},
        {"NUM_4", SIMPK_4},
        {"NUM_5", SIMPK_5},
        {"NUM_6", SIMPK_6},
        {"NUM_7", SIMPK_7},
        {"NUM_8", SIMPK_8},
        {"NUM_9", SIMPK_9},
        {"MINUS", SIMPK_MINUS},
        {"LEFTBRACKET", SIMPK_LEFTBRACKET},
        {"RIGHTBRACKET", SIMPK_RIGHTBRACKET},
        {"GREATER", SIMPK_GREATER},
        {"LESS", SIMPK_LESS},
        {"SPACE", SIMPK_SPACE},
        {"RETURN", SIMPK_RETURN},
        {"ESCAPE", SIMPK_ESCAPE},
        {"BACK", SIMPK_BACK},
        {"BACKSPACE", SIMPK_BACKSPACE},
        {"TAB", SIMPK_TAB},
        {"LEFT", SIMPK_LEFT},
        {"RIGHT", SIMPK_RIGHT},
        {"UP", SIMPK_UP},
        {"DOWN", SIMPK_DOWN},
        {"LCTRL", SIMPK_LCTRL},
        {"RCTRL", SIMPK_RCTRL},
        {"LSHIFT", SIMPK_LSHIFT},
        {"RSHIFT", SIMPK_RSHIFT},
        {"LALT", SIMPK_LALT},
        {"RALT", SIMPK_RALT},
        {"LGUI", SIMPK_LGUI},
        {"RGUI", SIMPK_RGUI},
        {"INSERT", SIMPK_INSERT},
        {"DELETE", SIMPK_DELETE},
        {"HOME", SIMPK_HOME},
        {"END", SIMPK_END},
        {"PAGEUP", SIMPK_PAGEUP},
        {"PAGEDOWN", SIMPK_PAGEDOWN},
        {"CAPSLOCK", SIMPK_CAPSLOCK},
        {"SCROLLLOCK", SIMPK_SCROLLLOCK},
        {"PRINTSCREEN", SIMPK_PRINTSCREEN},
        {"PAUSE", SIMPK_PAUSE},
        {"NUMLOCKCLEAR", SIMPK_NUMLOCKCLEAR},
        {"APPLICATION", SIMPK_APPLICATION},
        {"POWER", SIMPK_POWER},
        {"COMMA", SIMPK_COOMMA},
        {"SEMICOLON", SIMPK_SEMICOLON},
        {"PERIOD", SIMPK_PERIOD},
        {"APOSTROPHE", SIMPK_APOSTROPHE},
        {"LEFTBRACE", SIMPK_LEFTBRACE},
        {"RIGHTBRACE", SIMPK_RIGHTBRACE},
        {"SLASH", SIMPK_SLASH},
        {"BACKSLASH", SIMPK_BACKSLASH},
        {"EQUALS", SIMPK_EQUALS},
        {"GRAVE", SIMPK_GRAVE},
        {"TILDE", SIMPK_TILDE},
        {"F1", SIMPK_F1},
        {"F2", SIMPK_F2},
        {"F3", SIMPK_F3},
        {"F4", SIMPK_F4},
        {"F5", SIMPK_F5},
        {"F6", SIMPK_F6},
        {"F7", SIMPK_F7},
        {"F8", SIMPK_F8},
        {"F9", SIMPK_F9},
        {"F10", SIMPK_F10},
        {"F11", SIMPK_F11},
        {"F12", SIMPK_F12},
        {"MOUSE_LEFT", SIMPK_MOUSE_LEFT},
        {"MOUSE_RIGHT", SIMPK_MOUSE_RIGHT},
        {"MOUSE_MIDDLE", SIMPK_MOUSE_MIDDLE},
        {"MOUSE_X1", SIMPK_MOUSE_X1},
        {"MOUSE_X2", SIMPK_MOUSE_X2},
        };

        int Emit(lua_State *L, spdlog::level::level_enum level)
        {
            const int top = lua_gettop(L);
            luaL_Buffer buffer;
            luaL_buffinit(L, &buffer);
            for (int i = 1; i <= top; ++i)
            {
                if (i > 1)
                    luaL_addchar(&buffer, '\t');
                luaL_tolstring(L, i, nullptr);
                luaL_addvalue(&buffer);
            }
            luaL_pushresult(&buffer);
            Log::GetClientLogger()->log(level, "{}", lua_tostring(L, -1));
            return 0;
        }

        int LogTrace(lua_State *L) { return Emit(L, spdlog::level::trace); }
        int LogInfo(lua_State *L) { return Emit(L, spdlog::level::info); }
        int LogWarn(lua_State *L) { return Emit(L, spdlog::level::warn); }
        int LogError(lua_State *L) { return Emit(L, spdlog::level::err); }

        int InputIsKeyDown(lua_State *L)
        {
            lua_pushboolean(L, Input::IsKeyPressed(static_cast<int>(luaL_checkinteger(L, 1))));
            return 1;
        }

        int InputIsKeyJustPressed(lua_State *L)
        {
            lua_pushboolean(L, Input::IsKeyJustPressed(static_cast<int>(luaL_checkinteger(L, 1))));
            return 1;
        }

        int InputIsKeyJustReleased(lua_State *L)
        {
            lua_pushboolean(L, Input::IsKeyJustReleased(static_cast<int>(luaL_checkinteger(L, 1))));
            return 1;
        }

        int InputIsMouseDown(lua_State *L)
        {
            lua_pushboolean(L, Input::IsMouseButtonPressed(static_cast<int>(luaL_checkinteger(L, 1))));
            return 1;
        }

        int InputIsMouseJustPressed(lua_State *L)
        {
            lua_pushboolean(L, Input::IsMouseButtonJustPressed(static_cast<int>(luaL_checkinteger(L, 1))));
            return 1;
        }

        int InputIsMouseJustReleased(lua_State *L)
        {
            lua_pushboolean(L, Input::IsMouseButtonJustReleased(static_cast<int>(luaL_checkinteger(L, 1))));
            return 1;
        }

        int InputIsMouseInViewport(lua_State *L)
        {
            lua_pushboolean(L, Input::IsMouseInViewport());
            return 1;
        }

        int InputGetMouseWheel(lua_State *L)
        {
            lua_pushnumber(L, Input::GetMouseWheel());
            return 1;
        }

        int InputGetAxis(lua_State *L)
        {
            if (lua_type(L, 1) == LUA_TSTRING)
            {
                lua_pushnumber(L, InputActions::GetAxis(lua_tostring(L, 1)));
                return 1;
            }
            const int negative = static_cast<int>(luaL_checkinteger(L, 1));
            const int positive = static_cast<int>(luaL_checkinteger(L, 2));
            lua_Number value = 0.0;
            if (Input::IsKeyPressed(positive))
                value += 1.0;
            if (Input::IsKeyPressed(negative))
                value -= 1.0;
            lua_pushnumber(L, value);
            return 1;
        }

        int EntityIsValid(lua_State *L)
        {
            lua_pushboolean(L, static_cast<bool>(*CheckEntity(L, 1)));
            return 1;
        }

        int EntityGetName(lua_State *L)
        {
            Entity *entity = CheckLiveEntity(L, 1);
            if (!entity->HasComponent<TagComponent>())
            {
                lua_pushstring(L, "");
                return 1;
            }
            const std::string &tag = entity->GetComponent<TagComponent>().Tag;
            lua_pushlstring(L, tag.data(), tag.size());
            return 1;
        }

        int EntitySetName(lua_State *L)
        {
            Entity *entity = CheckLiveEntity(L, 1);
            size_t length = 0;
            const char *name = luaL_checklstring(L, 2, &length);
            auto &registry = entity->GetScene()->GetRegistry();
            registry.get_or_emplace<TagComponent>(entity->GetHandle()).Tag.assign(name, length);
            return 0;
        }

        int EntityGetPosition(lua_State *L)
        {
            const TransformComponent *transform = CheckTransform(L, 1);
            lua_pushnumber(L, transform->position.x);
            lua_pushnumber(L, transform->position.y);
            return 2;
        }

        int EntityGetWorldPosition(lua_State *L)
        {
            const TransformComponent *transform = CheckTransform(L, 1);
            const WorldTransform2D world = DecomposeWorld(*transform);
            lua_pushnumber(L, world.Position.x);
            lua_pushnumber(L, world.Position.y);
            return 2;
        }

        int EntitySetPosition(lua_State *L)
        {
            Entity *entity = nullptr;
            TransformComponent *transform = CheckTransform(L, 1, &entity);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            transform->position = {x, y};
            transform->dirty = true;
            SyncBody(entity, *transform);
            return 0;
        }

        int EntityTranslate(lua_State *L)
        {
            Entity *entity = nullptr;
            TransformComponent *transform = CheckTransform(L, 1, &entity);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            transform->position += glm::vec2(x, y);
            transform->dirty = true;
            SyncBody(entity, *transform);
            return 0;
        }

        int EntityGetRotation(lua_State *L)
        {
            lua_pushnumber(L, CheckTransform(L, 1)->rotation);
            return 1;
        }

        int EntitySetRotation(lua_State *L)
        {
            Entity *entity = nullptr;
            TransformComponent *transform = CheckTransform(L, 1, &entity);
            transform->rotation = static_cast<float>(luaL_checknumber(L, 2));
            transform->dirty = true;
            SyncBody(entity, *transform);
            return 0;
        }

        int EntityGetScale(lua_State *L)
        {
            const TransformComponent *transform = CheckTransform(L, 1);
            lua_pushnumber(L, transform->scale.x);
            lua_pushnumber(L, transform->scale.y);
            return 2;
        }

        int EntitySetScale(lua_State *L)
        {
            TransformComponent *transform = CheckTransform(L, 1);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_optnumber(L, 3, x));
            transform->scale = {x, y};
            transform->dirty = true;
            return 0;
        }

        int EntityGetZIndex(lua_State *L)
        {
            lua_pushnumber(L, CheckTransform(L, 1)->zIndex);
            return 1;
        }

        int EntitySetZIndex(lua_State *L)
        {
            TransformComponent *transform = CheckTransform(L, 1);
            transform->zIndex = static_cast<float>(luaL_checknumber(L, 2));
            transform->dirty = true;
            return 0;
        }

        int EntityGetMouseWorld(lua_State *L)
        {
            Entity *entity = CheckLiveEntity(L, 1);
            const glm::vec2 world = entity->GetScene()->ScreenToWorld(Input::GetMouseViewportPosition());
            lua_pushnumber(L, world.x);
            lua_pushnumber(L, world.y);
            return 2;
        }

        int EntityFindByName(lua_State *L)
        {
            Entity *entity = CheckLiveEntity(L, 1);
            size_t length = 0;
            const char *name = luaL_checklstring(L, 2, &length);

            Scene *scene = entity->GetScene();
            const entt::entity found = FindFirstByName(scene, std::string_view(name, length));
            if (found == entt::null)
            {
                lua_pushnil(L);
                return 1;
            }

            PushEntity(L, Entity(found, scene));
            return 1;
        }

        int EntityDestroy(lua_State *L)
        {
            Entity *entity = CheckLiveEntity(L, 1);
            float delay = static_cast<float>(luaL_optnumber(L, 2, 0.0));
            if (delay < 0.0f)
                delay = 0.0f;

            auto &registry = entity->GetScene()->GetRegistry();
            registry.emplace_or_replace<LifetimeComponent>(entity->GetHandle(), LifetimeComponent{delay});
            return 0;
        }

        int EntityGetParent(lua_State *L)
        {
            Entity *entity = CheckLiveEntity(L, 1);
            Scene *scene = entity->GetScene();
            auto &registry = scene->GetRegistry();
            const HierarchyComponent *hierarchy = registry.try_get<HierarchyComponent>(entity->GetHandle());
            if (!hierarchy || hierarchy->parent == entt::null || !registry.valid(hierarchy->parent))
            {
                lua_pushnil(L);
                return 1;
            }

            PushEntity(L, Entity(hierarchy->parent, scene));
            return 1;
        }

        int EntityGetChildren(lua_State *L)
        {
            Entity *entity = CheckLiveEntity(L, 1);
            Scene *scene = entity->GetScene();
            auto &registry = scene->GetRegistry();

            lua_newtable(L);
            const HierarchyComponent *hierarchy = registry.try_get<HierarchyComponent>(entity->GetHandle());
            if (!hierarchy)
                return 1;

            lua_Integer index = 1;
            for (entt::entity child : hierarchy->children)
            {
                if (!registry.valid(child))
                    continue;
                PushEntity(L, Entity(child, scene));
                lua_rawseti(L, -2, index++);
            }
            return 1;
        }

        int EntityGetScript(lua_State *L)
        {
            Entity *entity = CheckLiveEntity(L, 1);
            ScriptComponent *script = entity->GetScene()->GetRegistry().try_get<ScriptComponent>(entity->GetHandle());
            LuaScript *lua = script && script->Instance ? dynamic_cast<LuaScript *>(script->Instance.get()) : nullptr;
            if (!lua)
            {
                lua_pushnil(L);
                return 1;
            }

            lua->PushSelf(L);
            return 1;
        }

        int EntitySetScript(lua_State *L)
        {
            Entity *entity = CheckLiveEntity(L, 1);
            size_t length = 0;
            const char *path = luaL_checklstring(L, 2, &length);

            auto &registry = entity->GetScene()->GetRegistry();
            ScriptComponent *existing = registry.try_get<ScriptComponent>(entity->GetHandle());
            if (existing && existing->Instance)
                return luaL_error(L, "entity already has a running script");

            ScriptComponent &script = existing ? *existing : registry.emplace<ScriptComponent>(entity->GetHandle());
            script.BindLua(std::string(path, length));
            return 0;
        }

        int TimeNow(lua_State *L)
        {
            lua_pushnumber(L, s_ActiveScene ? s_ActiveScene->GetTime() : 0.0f);
            return 1;
        }

        int TimeDelta(lua_State *L)
        {
            lua_pushnumber(L, s_ActiveScene ? s_ActiveScene->GetDeltaTime() : 0.0f);
            return 1;
        }

        int TimeFixedDelta(lua_State *L)
        {
            lua_pushnumber(L, s_ActiveScene ? s_ActiveScene->GetFixedTimeStep() : 1.0f / 60.0f);
            return 1;
        }

        int SceneSpawn(lua_State *L)
        {
            Scene *scene = RequireScene(L);
            size_t length = 0;
            const char *name = luaL_checklstring(L, 1, &length);
            const float x = static_cast<float>(luaL_optnumber(L, 2, 0.0));
            const float y = static_cast<float>(luaL_optnumber(L, 3, 0.0));
            entt::entity parent = entt::null;
            if (!lua_isnoneornil(L, 4))
            {
                const Entity *parentEntity = CheckLiveEntity(L, 4);
                if (parentEntity->GetScene() != scene)
                    return luaL_error(L, "parent belongs to another scene");
                parent = parentEntity->GetHandle();
            }

            Entity entity = scene->CreateEntity(std::string(name, length));
            entity.GetComponent<TransformComponent>().position = {x, y};
            if (parent != entt::null)
                scene->SetParent(entity.GetHandle(), parent);

            PushEntity(L, entity);
            return 1;
        }

        int SceneClone(lua_State *L)
        {
            Scene *scene = RequireScene(L);
            entt::entity source = entt::null;
            if (lua_type(L, 1) == LUA_TSTRING)
            {
                size_t length = 0;
                const char *name = lua_tolstring(L, 1, &length);
                source = FindFirstByName(scene, std::string_view(name, length));
                if (source == entt::null)
                {
                    lua_pushnil(L);
                    return 1;
                }
            }
            else
            {
                const Entity *entity = CheckLiveEntity(L, 1);
                if (entity->GetScene() != scene)
                    return luaL_error(L, "entity belongs to another scene");
                source = entity->GetHandle();
            }
            const bool place = !lua_isnoneornil(L, 2);
            const float x = static_cast<float>(luaL_optnumber(L, 2, 0.0));
            const float y = static_cast<float>(luaL_optnumber(L, 3, 0.0));

            const entt::entity copy = SpawnerSystem::Clone(*scene, source);
            if (copy == entt::null)
            {
                lua_pushnil(L);
                return 1;
            }
            if (place)
            {
                auto &transform = scene->GetRegistry().get<TransformComponent>(copy);
                transform.position = {x, y};
                transform.dirty = true;
            }
            PushEntity(L, Entity(copy, scene));
            return 1;
        }

        int SceneFind(lua_State *L)
        {
            Scene *scene = RequireScene(L);
            size_t length = 0;
            const char *name = luaL_checklstring(L, 1, &length);
            const entt::entity found = FindFirstByName(scene, std::string_view(name, length));
            if (found == entt::null)
            {
                lua_pushnil(L);
                return 1;
            }

            PushEntity(L, Entity(found, scene));
            return 1;
        }

        int SceneFindAll(lua_State *L)
        {
            Scene *scene = RequireScene(L);
            size_t length = 0;
            const char *name = luaL_checklstring(L, 1, &length);
            const std::string_view wanted(name, length);

            lua_newtable(L);
            lua_Integer index = 1;
            auto view = scene->GetRegistry().view<TagComponent>();
            for (auto handle : view)
            {
                if (view.get<TagComponent>(handle).Tag != wanted)
                    continue;
                PushEntity(L, Entity(handle, scene));
                lua_rawseti(L, -2, index++);
            }
            return 1;
        }

        int SceneSetAmbient(lua_State *L)
        {
            Scene *scene = RequireScene(L);
            const Uint8 r = ToByte(L, 1);
            const Uint8 g = ToByte(L, 2);
            const Uint8 b = ToByte(L, 3);
            SceneLighting &lighting = scene->GetLighting();
            lighting.Ambient.r = static_cast<float>(r) / 255.0f;
            lighting.Ambient.g = static_cast<float>(g) / 255.0f;
            lighting.Ambient.b = static_cast<float>(b) / 255.0f;
            return 0;
        }

        int SceneSetLighting(lua_State *L)
        {
            Scene *scene = RequireScene(L);
            luaL_checkany(L, 1);
            scene->GetLighting().Enabled = lua_toboolean(L, 1) != 0;
            return 0;
        }

        int SceneInstantiate(lua_State *L)
        {
            Scene *scene = RequireScene(L);
            const char *path = luaL_checkstring(L, 1);
            const bool hasPosition = !lua_isnoneornil(L, 2);
            const glm::vec2 position(static_cast<float>(luaL_optnumber(L, 2, 0.0)), static_cast<float>(luaL_optnumber(L, 3, 0.0)));
            Entity *parentEntity = lua_isnoneornil(L, 4) ? nullptr : CheckLiveEntity(L, 4);
            entt::entity parent = parentEntity ? parentEntity->GetHandle() : entt::null;
            entt::entity root = entt::null;
            {
                const std::string prefabPath(path);
                root = Prefab::Instantiate(*scene, prefabPath, hasPosition ? std::optional<glm::vec2>(position) : std::nullopt, parent);
            }
            if (root == entt::null)
            {
                lua_pushnil(L);
                return 1;
            }
            PushEntity(L, Entity(root, scene));
            return 1;
        }

        float FadeOption(lua_State *L, int index, std::string &shader)
        {
            if (!lua_istable(L, index))
                return 0.0f;
            lua_getfield(L, index, "fade");
            const float fade = lua_isnumber(L, -1) ? static_cast<float>(lua_tonumber(L, -1)) : 0.0f;
            lua_pop(L, 1);
            lua_getfield(L, index, "shader");
            if (lua_type(L, -1) == LUA_TSTRING)
                shader = lua_tostring(L, -1);
            lua_pop(L, 1);
            return fade;
        }

        int SceneLoad(lua_State *L)
        {
            Scene *scene = RequireScene(L);
            const char *path = luaL_checkstring(L, 1);
            std::string shader;
            const float fade = FadeOption(L, 2, shader);
            scene->RequestLoad(path, fade, shader);
            return 0;
        }

        int SceneReload(lua_State *L)
        {
            Scene *scene = RequireScene(L);
            std::string shader;
            const float fade = FadeOption(L, 1, shader);
            scene->RequestReload(fade, shader);
            return 0;
        }

        int SceneGetPath(lua_State *L)
        {
            Scene *scene = RequireScene(L);
            lua_pushstring(L, scene->GetSourcePath().c_str());
            return 1;
        }

        int SceneIsTransitioning(lua_State *L)
        {
            Scene *scene = RequireScene(L);
            lua_pushboolean(L, scene->IsTransitioning());
            return 1;
        }

        int SceneSetFixedStep(lua_State *L)
        {
            Scene *scene = RequireScene(L);
            scene->SetFixedTimeStep(static_cast<float>(luaL_checknumber(L, 1)));
            return 0;
        }

        int SceneIsSolidAt(lua_State *L)
        {
            Scene *scene = RequireScene(L);
            const glm::vec2 point(static_cast<float>(luaL_checknumber(L, 1)), static_cast<float>(luaL_checknumber(L, 2)));
            lua_pushboolean(L, IsSolidAt(scene->GetRegistry(), point));
            return 1;
        }

        int SceneOverlapsSolid(lua_State *L)
        {
            Scene *scene = RequireScene(L);
            const float x1 = static_cast<float>(luaL_checknumber(L, 1));
            const float y1 = static_cast<float>(luaL_checknumber(L, 2));
            const float x2 = static_cast<float>(luaL_checknumber(L, 3));
            const float y2 = static_cast<float>(luaL_checknumber(L, 4));
            const glm::vec2 min(std::min(x1, x2), std::min(y1, y2));
            const glm::vec2 max(std::max(x1, x2), std::max(y1, y2));
            lua_pushboolean(L, OverlapsSolid(scene->GetRegistry(), min, max));
            return 1;
        }

        int AudioPlay(lua_State *L)
        {
            size_t length = 0;
            const char *path = luaL_checklstring(L, 1, &length);
            const float volume = static_cast<float>(luaL_optnumber(L, 2, 1.0));
            const float pitch = static_cast<float>(luaL_optnumber(L, 3, 1.0));
            const bool loop = lua_toboolean(L, 4) != 0;
            const char *busName = luaL_optstring(L, 5, "sfx");
            AudioBus bus = AudioBus::Sfx;
            bool knownBus = false;
            {
                knownBus = AudioEngine::BusFromName(busName, bus);
            }
            if (!knownBus)
                return luaL_error(L, "unknown audio bus (use sfx, music or ui)");
            const VoiceId voice = AudioEngine::Play(std::string(path, length), volume, pitch, loop, bus);
            lua_pushinteger(L, static_cast<lua_Integer>(voice));
            return 1;
        }

        int AudioSetBusVolume(lua_State *L)
        {
            const char *busName = luaL_checkstring(L, 1);
            const float volume = static_cast<float>(luaL_checknumber(L, 2));
            AudioBus bus = AudioBus::Sfx;
            bool knownBus = AudioEngine::BusFromName(busName, bus);
            if (!knownBus)
                return luaL_error(L, "unknown audio bus (use sfx, music or ui)");
            AudioEngine::SetBusVolume(bus, volume);
            return 0;
        }

        int AudioGetBusVolume(lua_State *L)
        {
            const char *busName = luaL_checkstring(L, 1);
            AudioBus bus = AudioBus::Sfx;
            bool knownBus = AudioEngine::BusFromName(busName, bus);
            if (!knownBus)
                return luaL_error(L, "unknown audio bus (use sfx, music or ui)");
            lua_pushnumber(L, AudioEngine::GetBusVolume(bus));
            return 1;
        }

        int AudioFadeTo(lua_State *L)
        {
            const VoiceId voice = static_cast<VoiceId>(luaL_checkinteger(L, 1));
            const float fade = static_cast<float>(luaL_checknumber(L, 2));
            const float seconds = static_cast<float>(luaL_optnumber(L, 3, 1.0));
            AudioEngine::FadeTo(voice, fade, seconds, lua_toboolean(L, 4) != 0);
            return 0;
        }

        int MusicPlay(lua_State *L)
        {
            size_t length = 0;
            const char *path = luaL_checklstring(L, 1, &length);
            const float volume = static_cast<float>(luaL_optnumber(L, 2, 1.0));
            const float fadeIn = static_cast<float>(luaL_optnumber(L, 3, 1.0));
            const float fadeOut = static_cast<float>(luaL_optnumber(L, 4, 1.0));
            const bool loop = lua_isnoneornil(L, 5) || lua_toboolean(L, 5) != 0;
            Music::Play(std::string(path, length), volume, fadeIn, fadeOut, loop);
            return 0;
        }

        int MusicStop(lua_State *L)
        {
            Music::Stop(static_cast<float>(luaL_optnumber(L, 1, 0.5)));
            return 0;
        }

        int MusicPause(lua_State *)
        {
            Music::Pause();
            return 0;
        }

        int MusicResume(lua_State *)
        {
            Music::Resume();
            return 0;
        }

        int MusicIsPlaying(lua_State *L)
        {
            lua_pushboolean(L, Music::IsPlaying() && !Music::IsPaused());
            return 1;
        }

        int MusicIsPaused(lua_State *L)
        {
            lua_pushboolean(L, Music::IsPaused());
            return 1;
        }

        int MusicGetCurrent(lua_State *L)
        {
            const std::string &current = Music::Current();
            lua_pushlstring(L, current.data(), current.size());
            return 1;
        }

        int MusicSetVolume(lua_State *L)
        {
            Music::SetVolume(static_cast<float>(luaL_checknumber(L, 1)));
            return 0;
        }

        int GameGet(lua_State *L)
        {
            size_t length = 0;
            const char *key = luaL_checklstring(L, 1, &length);
            const double fallback = luaL_optnumber(L, 2, 0.0);
            lua_pushnumber(L, GameVariables::Get(std::string(key, length), fallback));
            return 1;
        }

        int GameSet(lua_State *L)
        {
            size_t length = 0;
            const char *key = luaL_checklstring(L, 1, &length);
            const double value = luaL_checknumber(L, 2);
            GameVariables::Set(std::string(key, length), value);
            return 0;
        }

        int GameAdd(lua_State *L)
        {
            size_t length = 0;
            const char *key = luaL_checklstring(L, 1, &length);
            const double amount = luaL_optnumber(L, 2, 1.0);
            lua_pushnumber(L, GameVariables::Add(std::string(key, length), amount));
            return 1;
        }

        int GameHas(lua_State *L)
        {
            size_t length = 0;
            const char *key = luaL_checklstring(L, 1, &length);
            lua_pushboolean(L, GameVariables::Has(std::string(key, length)));
            return 1;
        }

        int GameRemove(lua_State *L)
        {
            size_t length = 0;
            const char *key = luaL_checklstring(L, 1, &length);
            GameVariables::Remove(std::string(key, length));
            return 0;
        }

        int GameReset(lua_State *)
        {
            GameVariables::Clear();
            return 0;
        }

        int GameSave(lua_State *L)
        {
            const std::string slot = luaL_optstring(L, 1, "save");
            lua_pushboolean(L, SaveData::Save(slot));
            return 1;
        }

        int GameLoad(lua_State *L)
        {
            const std::string slot = luaL_optstring(L, 1, "save");
            lua_pushboolean(L, SaveData::Load(slot));
            return 1;
        }

        int GameHasSave(lua_State *L)
        {
            const std::string slot = luaL_optstring(L, 1, "save");
            lua_pushboolean(L, SaveData::Exists(slot));
            return 1;
        }

        int GameDeleteSave(lua_State *L)
        {
            const std::string slot = luaL_optstring(L, 1, "save");
            lua_pushboolean(L, SaveData::Delete(slot));
            return 1;
        }

        int AudioStop(lua_State *L)
        {
            AudioEngine::Stop(static_cast<VoiceId>(luaL_checkinteger(L, 1)));
            return 0;
        }

        int AudioStopAll(lua_State *)
        {
            AudioEngine::StopAll();
            return 0;
        }

        int AudioIsPlaying(lua_State *L)
        {
            lua_pushboolean(L, AudioEngine::IsPlaying(static_cast<VoiceId>(luaL_checkinteger(L, 1))));
            return 1;
        }

        int AudioSetVolume(lua_State *L)
        {
            const VoiceId voice = static_cast<VoiceId>(luaL_checkinteger(L, 1));
            AudioEngine::SetVolume(voice, static_cast<float>(luaL_checknumber(L, 2)));
            return 0;
        }

        int AudioSetMasterVolume(lua_State *L)
        {
            AudioEngine::SetMasterVolume(static_cast<float>(luaL_checknumber(L, 1)));
            return 0;
        }

        int AudioGetMasterVolume(lua_State *L)
        {
            lua_pushnumber(L, AudioEngine::GetMasterVolume());
            return 1;
        }

        int MathLerp(lua_State *L)
        {
            const lua_Number a = luaL_checknumber(L, 1);
            const lua_Number b = luaL_checknumber(L, 2);
            const lua_Number t = luaL_checknumber(L, 3);
            lua_pushnumber(L, a + (b - a) * t);
            return 1;
        }

        int MathClamp(lua_State *L)
        {
            const lua_Number value = luaL_checknumber(L, 1);
            const lua_Number low = luaL_checknumber(L, 2);
            const lua_Number high = luaL_checknumber(L, 3);
            lua_pushnumber(L, value < low ? low : (value > high ? high : value));
            return 1;
        }

        int MathClamp01(lua_State *L)
        {
            const lua_Number value = luaL_checknumber(L, 1);
            lua_pushnumber(L, value < 0.0 ? 0.0 : (value > 1.0 ? 1.0 : value));
            return 1;
        }

        lua_Number Progress(lua_Number t, lua_Number a, lua_Number b)
        {
            if (b == a)
                return t >= a ? 1.0 : 0.0;
            const lua_Number v = (t - a) / (b - a);
            return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v);
        }

        int MathProgress(lua_State *L)
        {
            const lua_Number t = luaL_checknumber(L, 1);
            const lua_Number a = luaL_checknumber(L, 2);
            const lua_Number b = luaL_checknumber(L, 3);
            lua_pushnumber(L, Progress(t, a, b));
            return 1;
        }

        int MathRemap(lua_State *L)
        {
            const lua_Number value = luaL_checknumber(L, 1);
            const lua_Number inLow = luaL_checknumber(L, 2);
            const lua_Number inHigh = luaL_checknumber(L, 3);
            const lua_Number outLow = luaL_checknumber(L, 4);
            const lua_Number outHigh = luaL_checknumber(L, 5);
            const bool clamp = lua_toboolean(L, 6) != 0;
            lua_Number u = inHigh == inLow ? 0.0 : (value - inLow) / (inHigh - inLow);
            if (clamp)
                u = u < 0.0 ? 0.0 : (u > 1.0 ? 1.0 : u);
            lua_pushnumber(L, outLow + (outHigh - outLow) * u);
            return 1;
        }

        int MathSmoothstep(lua_State *L)
        {
            const lua_Number a = luaL_checknumber(L, 1);
            const lua_Number b = luaL_checknumber(L, 2);
            const lua_Number x = luaL_checknumber(L, 3);
            const lua_Number u = Progress(x, a, b);
            lua_pushnumber(L, u * u * (3.0 - 2.0 * u));
            return 1;
        }

        int MathWindow(lua_State *L)
        {
            const lua_Number t = luaL_checknumber(L, 1);
            const lua_Number a = luaL_checknumber(L, 2);
            const lua_Number b = luaL_checknumber(L, 3);
            const lua_Number fade = luaL_optnumber(L, 4, 0.4);
            const lua_Number in = Progress(t, a, a + fade);
            const lua_Number out = 1.0 - Progress(t, b - fade, b);
            lua_pushnumber(L, in < out ? in : out);
            return 1;
        }

        int MathSign(lua_State *L)
        {
            const lua_Number value = luaL_checknumber(L, 1);
            lua_pushnumber(L, value > 0.0 ? 1.0 : (value < 0.0 ? -1.0 : 0.0));
            return 1;
        }

        int MathMoveTowards(lua_State *L)
        {
            const lua_Number current = luaL_checknumber(L, 1);
            const lua_Number target = luaL_checknumber(L, 2);
            const lua_Number step = std::abs(luaL_checknumber(L, 3));
            const lua_Number difference = target - current;
            if (std::abs(difference) <= step)
                lua_pushnumber(L, target);
            else
                lua_pushnumber(L, current + (difference > 0.0 ? step : -step));
            return 1;
        }

        int MathLength(lua_State *L)
        {
            const lua_Number x = luaL_checknumber(L, 1);
            const lua_Number y = luaL_checknumber(L, 2);
            lua_pushnumber(L, std::sqrt(x * x + y * y));
            return 1;
        }

        int MathDistance(lua_State *L)
        {
            const lua_Number dx = luaL_checknumber(L, 3) - luaL_checknumber(L, 1);
            const lua_Number dy = luaL_checknumber(L, 4) - luaL_checknumber(L, 2);
            lua_pushnumber(L, std::sqrt(dx * dx + dy * dy));
            return 1;
        }

        int MathNormalize(lua_State *L)
        {
            const lua_Number x = luaL_checknumber(L, 1);
            const lua_Number y = luaL_checknumber(L, 2);
            const lua_Number length = std::sqrt(x * x + y * y);
            if (length <= 0.0)
            {
                lua_pushnumber(L, 0.0);
                lua_pushnumber(L, 0.0);
            }
            else
            {
                lua_pushnumber(L, x / length);
                lua_pushnumber(L, y / length);
            }
            return 2;
        }

        int MathAngle(lua_State *L)
        {
            const lua_Number dx = luaL_checknumber(L, 3) - luaL_checknumber(L, 1);
            const lua_Number dy = luaL_checknumber(L, 4) - luaL_checknumber(L, 2);
            lua_pushnumber(L, std::atan2(dy, dx) * 180.0 / 3.14159265358979323846);
            return 1;
        }

        int EntityEquals(lua_State *L)
        {
            const Entity *a = CheckEntity(L, 1);
            const Entity *b = CheckEntity(L, 2);
            lua_pushboolean(L, a->GetHandle() == b->GetHandle() && a->GetScene() == b->GetScene());
            return 1;
        }

        int EntityToString(lua_State *L)
        {
            Entity *entity = CheckEntity(L, 1);
            if (!static_cast<bool>(*entity))
            {
                lua_pushstring(L, "Entity(invalid)");
                return 1;
            }

            const char *name = "";
            if (entity->HasComponent<TagComponent>())
                name = entity->GetComponent<TagComponent>().Tag.c_str();
            lua_pushfstring(L, "Entity(%s)", name);
            return 1;
        }

        const ComponentBinding *FindBinding(const char *name)
        {
            for (const ComponentBinding *binding : ComponentBindings())
            {
                if (std::string_view(binding->Name) == name)
                    return binding;
            }
            return nullptr;
        }

        int EntityGet(lua_State *L)
        {
            Entity *entity = CheckLiveEntity(L, 1);
            const char *name = luaL_checkstring(L, 2);
            const ComponentBinding *binding = FindBinding(name);
            if (!binding)
                return luaL_error(L, "unknown component '%s'", name);
            if (!binding->Has(*entity))
            {
                lua_pushnil(L);
                return 1;
            }

            PushHandle(L, *entity, binding->Meta);
            return 1;
        }

        int EntityHas(lua_State *L)
        {
            Entity *entity = CheckLiveEntity(L, 1);
            const char *name = luaL_checkstring(L, 2);
            const ComponentBinding *binding = FindBinding(name);
            if (!binding)
                return luaL_error(L, "unknown component '%s'", name);
            lua_pushboolean(L, binding->Has(*entity));
            return 1;
        }

        int EntityAdd(lua_State *L)
        {
            Entity *entity = CheckLiveEntity(L, 1);
            const char *name = luaL_checkstring(L, 2);
            const ComponentBinding *binding = FindBinding(name);
            if (!binding)
                return luaL_error(L, "unknown component '%s'", name);
            if (!binding->Has(*entity))
            {
                if (!binding->Add)
                    return luaL_error(L, "component '%s' cannot be added from a script", name);
                binding->Add(*entity);
            }

            PushHandle(L, *entity, binding->Meta);
            return 1;
        }

        int EntityRemove(lua_State *L)
        {
            Entity *entity = CheckLiveEntity(L, 1);
            const char *name = luaL_checkstring(L, 2);
            const ComponentBinding *binding = FindBinding(name);
            if (!binding)
                return luaL_error(L, "unknown component '%s'", name);
            if (binding->Has(*entity))
                binding->Remove(*entity);
            return 0;
        }

        int EntityGetNamed(lua_State *L)
        {
            const lua_Integer index = lua_tointeger(L, lua_upvalueindex(1));
            const ComponentBinding &binding = *ComponentBindings()[static_cast<size_t>(index)];
            Entity *entity = CheckLiveEntity(L, 1);
            if (!binding.Has(*entity))
            {
                lua_pushnil(L);
                return 1;
            }

            PushHandle(L, *entity, binding.Meta);
            return 1;
        }

        const luaL_Reg LogFunctions[] = {
            {"trace", LogTrace},
            {"info", LogInfo},
            {"warn", LogWarn},
            {"error", LogError},
            {nullptr, nullptr}};

        const luaL_Reg InputFunctions[] = {
            {"IsKeyDown", InputIsKeyDown},
            {"IsKeyJustPressed", InputIsKeyJustPressed},
            {"IsKeyJustReleased", InputIsKeyJustReleased},
            {"IsMouseDown", InputIsMouseDown},
            {"IsMouseJustPressed", InputIsMouseJustPressed},
            {"IsMouseJustReleased", InputIsMouseJustReleased},
            {"IsMouseInViewport", InputIsMouseInViewport},
            {"GetMouseWheel", InputGetMouseWheel},
            {"GetAxis", InputGetAxis},
            {nullptr, nullptr}};

        const luaL_Reg TimeFunctions[] = {
            {"Now", TimeNow},
            {"Delta", TimeDelta},
            {"FixedDelta", TimeFixedDelta},
            {nullptr, nullptr}};

        const luaL_Reg SceneFunctions[] = {
            {"Spawn", SceneSpawn},
            {"Clone", SceneClone},
            {"Find", SceneFind},
            {"FindAll", SceneFindAll},
            {"SetAmbient", SceneSetAmbient},
            {"SetLighting", SceneSetLighting},
            {"Instantiate", SceneInstantiate},
            {"Load", SceneLoad},
            {"Reload", SceneReload},
            {"GetPath", SceneGetPath},
            {"IsTransitioning", SceneIsTransitioning},
            {"SetFixedStep", SceneSetFixedStep},
            {"IsSolidAt", SceneIsSolidAt},
            {"OverlapsSolid", SceneOverlapsSolid},
            {nullptr, nullptr}};

        int WindowSetRenderScale(lua_State *L)
        {
            DisplaySettings display = Display::Get();
            display.RenderScale = static_cast<float>(luaL_checknumber(L, 1));
            Display::Set(display);
            return 0;
        }

        int WindowGetRenderScale(lua_State *L)
        {
            lua_pushnumber(L, Display::Get().RenderScale);
            return 1;
        }

        int WindowSetFullscreen(lua_State *L)
        {
            luaL_checkany(L, 1);
            const bool fullscreen = lua_toboolean(L, 1) != 0;
            if (Application *app = Application::TryGet())
                app->SetFullscreen(fullscreen);
            return 0;
        }

        int WindowIsFullscreen(lua_State *L)
        {
            Application *app = Application::TryGet();
            lua_pushboolean(L, app && app->IsFullscreen());
            return 1;
        }

        int WindowGetSize(lua_State *L)
        {
            int width = 0;
            int height = 0;
            if (Application *app = Application::TryGet())
                SDL_GetWindowSize(app->GetWindow().GetNativeWindow(), &width, &height);
            lua_pushinteger(L, width);
            lua_pushinteger(L, height);
            return 2;
        }

        int WindowGetSafeArea(lua_State *L)
        {
            int width = 0;
            int height = 0;
            SDL_Rect safe{0, 0, 0, 0};
            if (Application *app = Application::TryGet())
            {
                SDL_Window *window = app->GetWindow().GetNativeWindow();
                SDL_GetWindowSize(window, &width, &height);
                if (!SDL_GetWindowSafeArea(window, &safe))
                    safe = {0, 0, width, height};
            }
            const glm::vec2 viewportSize = Input::GetViewportSize();
            const glm::vec2 origin = Input::WindowToViewport({0.0f, 0.0f});
            const float left = std::max(static_cast<float>(safe.x) + origin.x, 0.0f);
            const float top = std::max(static_cast<float>(safe.y) + origin.y, 0.0f);
            const float right = std::min(static_cast<float>(safe.x + safe.w) + origin.x, viewportSize.x);
            const float bottom = std::min(static_cast<float>(safe.y + safe.h) + origin.y, viewportSize.y);
            lua_pushnumber(L, left);
            lua_pushnumber(L, top);
            lua_pushnumber(L, std::max(right - left, 0.0f));
            lua_pushnumber(L, std::max(bottom - top, 0.0f));
            return 4;
        }

        const luaL_Reg WindowFunctions[] = {
            {"GetSafeArea", WindowGetSafeArea},
            {"SetFullscreen", WindowSetFullscreen},
            {"SetRenderScale", WindowSetRenderScale},
            {"GetRenderScale", WindowGetRenderScale},
            {"IsFullscreen", WindowIsFullscreen},
            {"GetSize", WindowGetSize},
            {nullptr, nullptr}};

        const luaL_Reg MusicFunctions[] = {
            {"Play", MusicPlay},
            {"Stop", MusicStop},
            {"Pause", MusicPause},
            {"Resume", MusicResume},
            {"IsPlaying", MusicIsPlaying},
            {"IsPaused", MusicIsPaused},
            {"GetCurrent", MusicGetCurrent},
            {"SetVolume", MusicSetVolume},
            {nullptr, nullptr}};

        int GameQuit(lua_State *)
        {
            if (Application *app = Application::TryGet())
                app->Queue([app] { app->RequestQuit(); });
            return 0;
        }

        const luaL_Reg GameFunctions[] = {
            {"Quit", GameQuit},
            {"Get", GameGet},
            {"Set", GameSet},
            {"Add", GameAdd},
            {"Has", GameHas},
            {"Remove", GameRemove},
            {"Reset", GameReset},
            {"Save", GameSave},
            {"Load", GameLoad},
            {"HasSave", GameHasSave},
            {"DeleteSave", GameDeleteSave},
            {nullptr, nullptr}};

        const luaL_Reg AudioFunctions[] = {
            {"Play", AudioPlay},
            {"Stop", AudioStop},
            {"StopAll", AudioStopAll},
            {"IsPlaying", AudioIsPlaying},
            {"SetVolume", AudioSetVolume},
            {"SetBusVolume", AudioSetBusVolume},
            {"GetBusVolume", AudioGetBusVolume},
            {"FadeTo", AudioFadeTo},
            {"SetMasterVolume", AudioSetMasterVolume},
            {"GetMasterVolume", AudioGetMasterVolume},
            {nullptr, nullptr}};

        const luaL_Reg MathFunctions[] = {
            {"Lerp", MathLerp},
            {"Clamp", MathClamp},
            {"Clamp01", MathClamp01},
            {"Progress", MathProgress},
            {"Remap", MathRemap},
            {"Smoothstep", MathSmoothstep},
            {"Window", MathWindow},
            {"Sign", MathSign},
            {"MoveTowards", MathMoveTowards},
            {"Length", MathLength},
            {"Distance", MathDistance},
            {"Normalize", MathNormalize},
            {"Angle", MathAngle},
            {nullptr, nullptr}};

        const luaL_Reg EntityMethods[] = {
            {"IsValid", EntityIsValid},
            {"GetName", EntityGetName},
            {"SetName", EntitySetName},
            {"GetPosition", EntityGetPosition},
            {"GetWorldPosition", EntityGetWorldPosition},
            {"SetPosition", EntitySetPosition},
            {"Translate", EntityTranslate},
            {"GetRotation", EntityGetRotation},
            {"SetRotation", EntitySetRotation},
            {"GetScale", EntityGetScale},
            {"SetScale", EntitySetScale},
            {"GetZIndex", EntityGetZIndex},
            {"SetZIndex", EntitySetZIndex},
            {"GetMouseWorld", EntityGetMouseWorld},
            {"FindByName", EntityFindByName},
            {"Destroy", EntityDestroy},
            {"GetParent", EntityGetParent},
            {"GetChildren", EntityGetChildren},
            {"GetScript", EntityGetScript},
            {"SetScript", EntitySetScript},
            {"Get", EntityGet},
            {"Has", EntityHas},
            {"Add", EntityAdd},
            {"Remove", EntityRemove},
            {nullptr, nullptr}};

        void RegisterKeys(lua_State *L)
        {
            lua_createtable(L, 0, static_cast<int>(sizeof(Keys) / sizeof(Keys[0])));
            for (const KeyEntry &key : Keys)
            {
                lua_pushinteger(L, key.Code);
                lua_setfield(L, -2, key.Name);
            }
            lua_setglobal(L, "Key");
        }

        void RegisterHandle(lua_State *L, const char *meta, const luaL_Reg *methods)
        {
            luaL_newmetatable(L, meta);
            lua_newtable(L);
            luaL_setfuncs(L, methods, 0);
            lua_setfield(L, -2, "__index");
            lua_pop(L, 1);
        }

        void RegisterEntity(lua_State *L)
        {
            luaL_newmetatable(L, EntityMeta);

            luaL_newlib(L, EntityMethods);
            const auto &bindings = ComponentBindings();
            for (size_t i = 0; i < bindings.size(); ++i)
            {
                const std::string method = std::string("Get") + bindings[i]->Name;
                lua_pushinteger(L, static_cast<lua_Integer>(i));
                lua_pushcclosure(L, EntityGetNamed, 1);
                lua_setfield(L, -2, method.c_str());
            }
            lua_setfield(L, -2, "__index");

            lua_pushcfunction(L, EntityEquals);
            lua_setfield(L, -2, "__eq");

            lua_pushcfunction(L, EntityToString);
            lua_setfield(L, -2, "__tostring");

            lua_pop(L, 1);
        }
    }

    void SetActiveScene(Scene *scene)
    {
        s_ActiveScene = scene;
    }

    Scene *GetActiveScene()
    {
        return s_ActiveScene;
    }

    void PushEntity(lua_State *L, const Entity &entity)
    {
        void *memory = lua_newuserdatauv(L, sizeof(Entity), 0);
        new (memory) Entity(entity);
        luaL_setmetatable(L, EntityMeta);
    }

    void Register(lua_State *L)
    {
        luaL_newlib(L, LogFunctions);
        lua_setglobal(L, "log");

        luaL_newlib(L, InputFunctions);
        lua_setglobal(L, "Input");

        luaL_newlib(L, TimeFunctions);
        lua_setglobal(L, "Time");

        luaL_newlib(L, SceneFunctions);
        lua_setglobal(L, "Scene");

        luaL_newlib(L, AudioFunctions);
        lua_setglobal(L, "Audio");

        luaL_newlib(L, WindowFunctions);
        lua_setglobal(L, "Window");

        luaL_newlib(L, MusicFunctions);
        lua_setglobal(L, "Music");

        luaL_newlib(L, GameFunctions);
        lua_setglobal(L, "Game");

        luaL_newlib(L, MathFunctions);
        lua_setglobal(L, "Math");

        lua_pushcfunction(L, LogInfo);
        lua_setglobal(L, "print");

        RegisterKeys(L);
        RegisterEntity(L);
        RegisterTween(L);
        RegisterPath(L);
        RegisterDraw(L);
        RegisterColor(L);
        RegisterEffects(L);
        RegisterPhysics(L);
        RegisterInputActions(L);
        for (const ComponentBinding *binding : ComponentBindings())
            RegisterHandle(L, binding->Meta, binding->Methods);
        RunPrelude(L);
    }
}
