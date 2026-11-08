#include "LuaBindings.h"
#include "LuaInclude.h"

#include "LuaScript.h"

#include <Engine/Audio/AudioEngine.h>
#include <Engine/Core/Log.h>
#include <Engine/Input/Input.h>
#include <Engine/Input/SIMP_Keys.h>
#include <Engine/Math/EntityUtility.h>
#include <Engine/Particles/ParticleEmitter.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/ScriptableEntity.h>

#include <cmath>
#include <new>
#include <string>
#include <string_view>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *EntityMeta = "SIMP.Entity";
        constexpr const char *SpriteMeta = "SIMP.Sprite";
        constexpr const char *CameraMeta = "SIMP.Camera";
        constexpr const char *LightMeta = "SIMP.Light";
        constexpr const char *VelocityMeta = "SIMP.Velocity";
        constexpr const char *CollisionMeta = "SIMP.Collision";
        constexpr const char *ShapeMeta = "SIMP.Shape";
        constexpr const char *TextMeta = "SIMP.Text";
        constexpr const char *GlowMeta = "SIMP.Glow";
        constexpr const char *ParticlesMeta = "SIMP.Particles";
        constexpr const char *AudioMeta = "SIMP.Audio";
        constexpr const char *ParallaxMeta = "SIMP.Parallax";
        constexpr const char *BodyMeta = "SIMP.Body";
        constexpr const char *TriggerMeta = "SIMP.Trigger";
        constexpr const char *TimerMeta = "SIMP.Timer";
        constexpr const char *PathMeta = "SIMP.Path";
        constexpr const char *HealthMeta = "SIMP.Health";

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

        Entity *CheckEntity(lua_State *L, int index)
        {
            return static_cast<Entity *>(luaL_checkudata(L, index, EntityMeta));
        }

        Entity *CheckLiveEntity(lua_State *L, int index)
        {
            Entity *entity = CheckEntity(L, index);
            if (!static_cast<bool>(*entity))
                luaL_error(L, "entity is no longer valid");
            return entity;
        }

        Scene *RequireScene(lua_State *L)
        {
            if (!s_ActiveScene)
                luaL_error(L, "no active scene");
            return s_ActiveScene;
        }

        TransformComponent *CheckTransform(lua_State *L, int index, Entity **entityOut = nullptr)
        {
            Entity *entity = CheckLiveEntity(L, index);
            if (!entity->HasComponent<TransformComponent>())
                luaL_error(L, "entity has no transform");
            if (entityOut)
                *entityOut = entity;
            return &entity->GetComponent<TransformComponent>();
        }

        void SyncBody(Entity *entity, const TransformComponent &transform)
        {
            PhysicsComponent *physics = entity->GetScene()->GetRegistry().try_get<PhysicsComponent>(entity->GetHandle());
            if (!physics || !physics->body)
                return;

            physics->body->position = {transform.position.x, transform.position.y};
            physics->body->orientation = glm::radians(transform.rotation);
            physics->body->calculateDerivativeData();
        }

        entt::entity FindFirstByName(Scene *scene, std::string_view wanted)
        {
            auto view = scene->GetRegistry().view<TagComponent>();
            for (auto handle : view)
            {
                if (view.get<TagComponent>(handle).Tag == wanted)
                    return handle;
            }
            return entt::null;
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

        template <typename T>
        Entity *CheckHandleEntity(lua_State *L, const char *meta)
        {
            Entity *entity = static_cast<Entity *>(luaL_checkudata(L, 1, meta));
            if (!static_cast<bool>(*entity))
                luaL_error(L, "entity is no longer valid");
            if (!entity->HasComponent<T>())
                luaL_error(L, "entity no longer has this component");
            return entity;
        }

        template <typename T>
        T *CheckComponent(lua_State *L, const char *meta)
        {
            Entity *entity = CheckHandleEntity<T>(L, meta);
            return &entity->GetComponent<T>();
        }

        void PushHandle(lua_State *L, const Entity &entity, const char *meta)
        {
            void *memory = lua_newuserdatauv(L, sizeof(Entity), 0);
            new (memory) Entity(entity);
            luaL_setmetatable(L, meta);
        }

        Uint8 ToByte(lua_State *L, int index)
        {
            const lua_Number value = luaL_checknumber(L, index);
            if (value <= 0.0)
                return 0;
            if (value >= 255.0)
                return 255;
            return static_cast<Uint8>(value);
        }

        Uint8 ToByteOpt(lua_State *L, int index, Uint8 fallback)
        {
            if (lua_isnoneornil(L, index))
                return fallback;
            return ToByte(L, index);
        }

        int PushRgba(lua_State *L, const SDL_Color &color)
        {
            lua_pushinteger(L, color.r);
            lua_pushinteger(L, color.g);
            lua_pushinteger(L, color.b);
            lua_pushinteger(L, color.a);
            return 4;
        }

        void ReadRgba(lua_State *L, int first, SDL_Color &color)
        {
            const Uint8 r = ToByte(L, first);
            const Uint8 g = ToByte(L, first + 1);
            const Uint8 b = ToByte(L, first + 2);
            const Uint8 a = ToByteOpt(L, first + 3, color.a);
            color = SDL_Color{r, g, b, a};
        }

        int SpriteGetTint(lua_State *L)
        {
            return PushRgba(L, CheckComponent<SpriteComponent>(L, SpriteMeta)->tint);
        }

        int SpriteSetTint(lua_State *L)
        {
            SpriteComponent *sprite = CheckComponent<SpriteComponent>(L, SpriteMeta);
            ReadRgba(L, 2, sprite->tint);
            return 0;
        }

        int SpriteGetFlip(lua_State *L)
        {
            const SpriteComponent *sprite = CheckComponent<SpriteComponent>(L, SpriteMeta);
            lua_pushboolean(L, sprite->flipX);
            lua_pushboolean(L, sprite->flipY);
            return 2;
        }

        int SpriteSetFlip(lua_State *L)
        {
            SpriteComponent *sprite = CheckComponent<SpriteComponent>(L, SpriteMeta);
            luaL_checkany(L, 2);
            luaL_checkany(L, 3);
            sprite->flipX = lua_toboolean(L, 2) != 0;
            sprite->flipY = lua_toboolean(L, 3) != 0;
            return 0;
        }

        int SpriteGetSize(lua_State *L)
        {
            const SpriteComponent *sprite = CheckComponent<SpriteComponent>(L, SpriteMeta);
            lua_pushnumber(L, sprite->width);
            lua_pushnumber(L, sprite->height);
            return 2;
        }

        int SpriteSetSize(lua_State *L)
        {
            SpriteComponent *sprite = CheckComponent<SpriteComponent>(L, SpriteMeta);
            const float width = static_cast<float>(luaL_checknumber(L, 2));
            const float height = static_cast<float>(luaL_checknumber(L, 3));
            sprite->width = width;
            sprite->height = height;
            return 0;
        }

        int SpriteSetRegion(lua_State *L)
        {
            SpriteComponent *sprite = CheckComponent<SpriteComponent>(L, SpriteMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            const float w = static_cast<float>(luaL_checknumber(L, 4));
            const float h = static_cast<float>(luaL_checknumber(L, 5));
            sprite->region = SDL_FRect{x, y, w, h};
            sprite->useRegion = true;
            return 0;
        }

        int SpriteClearRegion(lua_State *L)
        {
            CheckComponent<SpriteComponent>(L, SpriteMeta)->useRegion = false;
            return 0;
        }

        int CameraShake(lua_State *L)
        {
            CameraComponent *camera = CheckComponent<CameraComponent>(L, CameraMeta);
            camera->Shake(static_cast<float>(luaL_checknumber(L, 2)));
            return 0;
        }

        int CameraGetZoom(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<CameraComponent>(L, CameraMeta)->Camera.GetZoom());
            return 1;
        }

        int CameraSetZoom(lua_State *L)
        {
            CameraComponent *camera = CheckComponent<CameraComponent>(L, CameraMeta);
            const float zoom = static_cast<float>(luaL_checknumber(L, 2));
            camera->Camera.SetZoom(zoom);
            return 0;
        }

        int CameraSetFollow(lua_State *L)
        {
            CameraComponent *camera = CheckComponent<CameraComponent>(L, CameraMeta);
            size_t length = 0;
            const char *name = luaL_checklstring(L, 2, &length);
            if (!lua_isnoneornil(L, 3))
            {
                const float x = static_cast<float>(luaL_checknumber(L, 3));
                const float y = static_cast<float>(luaL_optnumber(L, 4, 0.0));
                camera->followOffset = {x, y};
            }
            camera->followTarget.assign(name, length);
            camera->followHandle = entt::null;
            return 0;
        }

        int CameraIsPrimary(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<CameraComponent>(L, CameraMeta)->primary);
            return 1;
        }

        int LightGetColor(lua_State *L)
        {
            const LightComponent *light = CheckComponent<LightComponent>(L, LightMeta);
            lua_pushinteger(L, light->color.r);
            lua_pushinteger(L, light->color.g);
            lua_pushinteger(L, light->color.b);
            return 3;
        }

        int LightSetColor(lua_State *L)
        {
            LightComponent *light = CheckComponent<LightComponent>(L, LightMeta);
            const Uint8 r = ToByte(L, 2);
            const Uint8 g = ToByte(L, 3);
            const Uint8 b = ToByte(L, 4);
            light->color = SDL_Color{r, g, b, light->color.a};
            return 0;
        }

        int LightGetIntensity(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<LightComponent>(L, LightMeta)->intensity);
            return 1;
        }

        int LightSetIntensity(lua_State *L)
        {
            LightComponent *light = CheckComponent<LightComponent>(L, LightMeta);
            light->intensity = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int LightGetRadius(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<LightComponent>(L, LightMeta)->radius);
            return 1;
        }

        int LightSetRadius(lua_State *L)
        {
            LightComponent *light = CheckComponent<LightComponent>(L, LightMeta);
            light->radius = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int LightIsEnabled(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<LightComponent>(L, LightMeta)->enabled);
            return 1;
        }

        int LightSetEnabled(lua_State *L)
        {
            LightComponent *light = CheckComponent<LightComponent>(L, LightMeta);
            luaL_checkany(L, 2);
            light->enabled = lua_toboolean(L, 2) != 0;
            return 0;
        }

        int VelocityGet(lua_State *L)
        {
            const VelocityComponent *velocity = CheckComponent<VelocityComponent>(L, VelocityMeta);
            lua_pushnumber(L, velocity->vx);
            lua_pushnumber(L, velocity->vy);
            return 2;
        }

        int VelocitySet(lua_State *L)
        {
            VelocityComponent *velocity = CheckComponent<VelocityComponent>(L, VelocityMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            velocity->vx = x;
            velocity->vy = y;
            return 0;
        }

        int VelocityAdd(lua_State *L)
        {
            VelocityComponent *velocity = CheckComponent<VelocityComponent>(L, VelocityMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            velocity->vx += x;
            velocity->vy += y;
            return 0;
        }

        int CollisionGetSize(lua_State *L)
        {
            const CollisionComponent *collision = CheckComponent<CollisionComponent>(L, CollisionMeta);
            lua_pushnumber(L, collision->width);
            lua_pushnumber(L, collision->height);
            return 2;
        }

        int CollisionSetSize(lua_State *L)
        {
            CollisionComponent *collision = CheckComponent<CollisionComponent>(L, CollisionMeta);
            const float width = static_cast<float>(luaL_checknumber(L, 2));
            const float height = static_cast<float>(luaL_checknumber(L, 3));
            collision->width = width;
            collision->height = height;
            return 0;
        }

        int CollisionGetOffset(lua_State *L)
        {
            const CollisionComponent *collision = CheckComponent<CollisionComponent>(L, CollisionMeta);
            lua_pushnumber(L, collision->offsetX);
            lua_pushnumber(L, collision->offsetY);
            return 2;
        }

        int CollisionSetOffset(lua_State *L)
        {
            CollisionComponent *collision = CheckComponent<CollisionComponent>(L, CollisionMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            collision->offsetX = x;
            collision->offsetY = y;
            return 0;
        }

        int CollisionGetBounds(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<CollisionComponent>(L, CollisionMeta);
            const CollisionComponent &collision = entity->GetComponent<CollisionComponent>();
            if (!entity->HasComponent<TransformComponent>())
                return luaL_error(L, "entity has no transform");
            const WorldTransform2D world = DecomposeWorld(entity->GetComponent<TransformComponent>());
            lua_pushnumber(L, world.Position.x + collision.offsetX);
            lua_pushnumber(L, world.Position.y + collision.offsetY);
            lua_pushnumber(L, collision.width * std::abs(world.Scale.x));
            lua_pushnumber(L, collision.height * std::abs(world.Scale.y));
            return 4;
        }

        int ShapeGetColor(lua_State *L)
        {
            return PushRgba(L, CheckComponent<RenderComponent>(L, ShapeMeta)->color);
        }

        int ShapeSetColor(lua_State *L)
        {
            RenderComponent *shape = CheckComponent<RenderComponent>(L, ShapeMeta);
            ReadRgba(L, 2, shape->color);
            return 0;
        }

        int ShapeGetSize(lua_State *L)
        {
            const RenderComponent *shape = CheckComponent<RenderComponent>(L, ShapeMeta);
            lua_pushnumber(L, shape->width);
            lua_pushnumber(L, shape->height);
            return 2;
        }

        int ShapeSetSize(lua_State *L)
        {
            RenderComponent *shape = CheckComponent<RenderComponent>(L, ShapeMeta);
            const float width = static_cast<float>(luaL_checknumber(L, 2));
            const float height = static_cast<float>(luaL_checknumber(L, 3));
            shape->width = width;
            shape->height = height;
            return 0;
        }

        int TextGetText(lua_State *L)
        {
            const TextComponent *text = CheckComponent<TextComponent>(L, TextMeta);
            lua_pushlstring(L, text->Text.data(), text->Text.size());
            return 1;
        }

        int TextSetText(lua_State *L)
        {
            TextComponent *text = CheckComponent<TextComponent>(L, TextMeta);
            size_t length = 0;
            const char *value = luaL_checklstring(L, 2, &length);
            text->Text.assign(value, length);
            return 0;
        }

        int TextGetFontSize(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<TextComponent>(L, TextMeta)->Size);
            return 1;
        }

        int TextSetFontSize(lua_State *L)
        {
            TextComponent *text = CheckComponent<TextComponent>(L, TextMeta);
            text->Size = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int TextGetColor(lua_State *L)
        {
            return PushRgba(L, CheckComponent<TextComponent>(L, TextMeta)->Color);
        }

        int TextSetColor(lua_State *L)
        {
            TextComponent *text = CheckComponent<TextComponent>(L, TextMeta);
            ReadRgba(L, 2, text->Color);
            return 0;
        }

        int TextGetAlign(lua_State *L)
        {
            lua_pushinteger(L, CheckComponent<TextComponent>(L, TextMeta)->Align);
            return 1;
        }

        int TextSetAlign(lua_State *L)
        {
            TextComponent *text = CheckComponent<TextComponent>(L, TextMeta);
            const lua_Integer align = luaL_checkinteger(L, 2);
            text->Align = static_cast<int>(align < 0 ? 0 : (align > 2 ? 2 : align));
            return 0;
        }

        int GlowGetColor(lua_State *L)
        {
            return PushRgba(L, CheckComponent<GlowComponent>(L, GlowMeta)->color);
        }

        int GlowSetColor(lua_State *L)
        {
            GlowComponent *glow = CheckComponent<GlowComponent>(L, GlowMeta);
            ReadRgba(L, 2, glow->color);
            return 0;
        }

        int GlowGetIntensity(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<GlowComponent>(L, GlowMeta)->intensity);
            return 1;
        }

        int GlowSetIntensity(lua_State *L)
        {
            GlowComponent *glow = CheckComponent<GlowComponent>(L, GlowMeta);
            glow->intensity = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int GlowGetSpread(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<GlowComponent>(L, GlowMeta)->spread);
            return 1;
        }

        int GlowSetSpread(lua_State *L)
        {
            GlowComponent *glow = CheckComponent<GlowComponent>(L, GlowMeta);
            glow->spread = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int GlowIsEnabled(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<GlowComponent>(L, GlowMeta)->enabled);
            return 1;
        }

        int GlowSetEnabled(lua_State *L)
        {
            GlowComponent *glow = CheckComponent<GlowComponent>(L, GlowMeta);
            luaL_checkany(L, 2);
            glow->enabled = lua_toboolean(L, 2) != 0;
            return 0;
        }

        int ParticlesPlay(lua_State *L)
        {
            CheckComponent<ParticleEmitterComponent>(L, ParticlesMeta)->playing = true;
            return 0;
        }

        int ParticlesStop(lua_State *L)
        {
            CheckComponent<ParticleEmitterComponent>(L, ParticlesMeta)->playing = false;
            return 0;
        }

        int ParticlesIsPlaying(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<ParticleEmitterComponent>(L, ParticlesMeta)->playing);
            return 1;
        }

        int ParticlesRestart(lua_State *L)
        {
            ParticleEmitterComponent *emitter = CheckComponent<ParticleEmitterComponent>(L, ParticlesMeta);
            ParticleEmitter::Restart(emitter->state);
            emitter->playing = true;
            return 0;
        }

        int ParticlesBurst(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<ParticleEmitterComponent>(L, ParticlesMeta);
            const int count = static_cast<int>(luaL_checkinteger(L, 2));
            TransformComponent *transform = entity->GetScene()->GetRegistry().try_get<TransformComponent>(entity->GetHandle());
            if (!transform || count <= 0)
                return 0;

            ParticleEmitterComponent &emitter = entity->GetComponent<ParticleEmitterComponent>();
            ParticleEmitter::Emit(emitter.effect, emitter.state, ParticleSystem::FrameOf(*transform), count);
            return 0;
        }

        int ParticlesGetRate(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<ParticleEmitterComponent>(L, ParticlesMeta)->effect.Rate);
            return 1;
        }

        int ParticlesSetRate(lua_State *L)
        {
            ParticleEmitterComponent *emitter = CheckComponent<ParticleEmitterComponent>(L, ParticlesMeta);
            emitter->effect.Rate = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int AudioSourcePlay(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<AudioSourceComponent>(L, AudioMeta);
            AudioSourceComponent &source = entity->GetComponent<AudioSourceComponent>();
            AudioSystem::StopSource(entity->GetScene()->GetRegistry(), entity->GetHandle());
            source.started = true;
            if (!source.clip.empty())
                source.voice = AudioEngine::Play(source.clip, source.volume, source.pitch, source.loop);
            return 0;
        }

        int AudioSourceStop(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<AudioSourceComponent>(L, AudioMeta);
            AudioSystem::StopSource(entity->GetScene()->GetRegistry(), entity->GetHandle());
            return 0;
        }

        int AudioSourceIsPlaying(lua_State *L)
        {
            const AudioSourceComponent *source = CheckComponent<AudioSourceComponent>(L, AudioMeta);
            lua_pushboolean(L, source->voice != 0 && AudioEngine::IsPlaying(source->voice));
            return 1;
        }

        int AudioSourceGetVolume(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<AudioSourceComponent>(L, AudioMeta)->volume);
            return 1;
        }

        int AudioSourceSetVolume(lua_State *L)
        {
            AudioSourceComponent *source = CheckComponent<AudioSourceComponent>(L, AudioMeta);
            source->volume = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int AudioSourceGetPitch(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<AudioSourceComponent>(L, AudioMeta)->pitch);
            return 1;
        }

        int AudioSourceSetPitch(lua_State *L)
        {
            AudioSourceComponent *source = CheckComponent<AudioSourceComponent>(L, AudioMeta);
            source->pitch = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int AudioSourceSetLoop(lua_State *L)
        {
            AudioSourceComponent *source = CheckComponent<AudioSourceComponent>(L, AudioMeta);
            luaL_checkany(L, 2);
            source->loop = lua_toboolean(L, 2) != 0;
            return 0;
        }

        int AudioSourceSetClip(lua_State *L)
        {
            AudioSourceComponent *source = CheckComponent<AudioSourceComponent>(L, AudioMeta);
            size_t length = 0;
            const char *clip = luaL_checklstring(L, 2, &length);
            source->clip.assign(clip, length);
            return 0;
        }

        int ParallaxGetFactor(lua_State *L)
        {
            const ParallaxComponent *parallax = CheckComponent<ParallaxComponent>(L, ParallaxMeta);
            lua_pushnumber(L, parallax->factor.x);
            lua_pushnumber(L, parallax->factor.y);
            return 2;
        }

        int ParallaxSetFactor(lua_State *L)
        {
            ParallaxComponent *parallax = CheckComponent<ParallaxComponent>(L, ParallaxMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_optnumber(L, 3, x));
            parallax->factor = {x, y};
            return 0;
        }

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

        int TriggerGetSize(lua_State *L)
        {
            const TriggerComponent *trigger = CheckComponent<TriggerComponent>(L, TriggerMeta);
            lua_pushnumber(L, trigger->width);
            lua_pushnumber(L, trigger->height);
            return 2;
        }

        int TriggerSetSize(lua_State *L)
        {
            TriggerComponent *trigger = CheckComponent<TriggerComponent>(L, TriggerMeta);
            const float width = static_cast<float>(luaL_checknumber(L, 2));
            const float height = static_cast<float>(luaL_checknumber(L, 3));
            trigger->width = width;
            trigger->height = height;
            return 0;
        }

        int TriggerGetOffset(lua_State *L)
        {
            const TriggerComponent *trigger = CheckComponent<TriggerComponent>(L, TriggerMeta);
            lua_pushnumber(L, trigger->offsetX);
            lua_pushnumber(L, trigger->offsetY);
            return 2;
        }

        int TriggerSetOffset(lua_State *L)
        {
            TriggerComponent *trigger = CheckComponent<TriggerComponent>(L, TriggerMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            trigger->offsetX = x;
            trigger->offsetY = y;
            return 0;
        }

        int TriggerGetFilter(lua_State *L)
        {
            const TriggerComponent *trigger = CheckComponent<TriggerComponent>(L, TriggerMeta);
            lua_pushlstring(L, trigger->filter.data(), trigger->filter.size());
            return 1;
        }

        int TriggerSetFilter(lua_State *L)
        {
            TriggerComponent *trigger = CheckComponent<TriggerComponent>(L, TriggerMeta);
            size_t length = 0;
            const char *filter = luaL_optlstring(L, 2, "", &length);
            trigger->filter.assign(filter, length);
            return 0;
        }

        int TriggerIsEnabled(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<TriggerComponent>(L, TriggerMeta)->enabled);
            return 1;
        }

        int TriggerSetEnabled(lua_State *L)
        {
            TriggerComponent *trigger = CheckComponent<TriggerComponent>(L, TriggerMeta);
            luaL_checkany(L, 2);
            trigger->enabled = lua_toboolean(L, 2) != 0;
            return 0;
        }

        int TriggerGetInside(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<TriggerComponent>(L, TriggerMeta);
            Scene *scene = entity->GetScene();
            auto &registry = scene->GetRegistry();
            const TriggerComponent &trigger = entity->GetComponent<TriggerComponent>();

            lua_newtable(L);
            lua_Integer index = 1;
            for (entt::entity other : trigger.inside)
            {
                if (!registry.valid(other))
                    continue;
                PushEntity(L, Entity(other, scene));
                lua_rawseti(L, -2, index++);
            }
            return 1;
        }

        int TriggerContains(lua_State *L)
        {
            const TriggerComponent *trigger = CheckComponent<TriggerComponent>(L, TriggerMeta);
            const Entity *other = CheckEntity(L, 2);
            bool found = false;
            for (entt::entity inside : trigger->inside)
            {
                if (inside == other->GetHandle())
                {
                    found = true;
                    break;
                }
            }
            lua_pushboolean(L, found);
            return 1;
        }

        int TimerStart(lua_State *L)
        {
            CheckComponent<TimerComponent>(L, TimerMeta)->running = true;
            return 0;
        }

        int TimerStop(lua_State *L)
        {
            CheckComponent<TimerComponent>(L, TimerMeta)->running = false;
            return 0;
        }

        int TimerRestart(lua_State *L)
        {
            TimerComponent *timer = CheckComponent<TimerComponent>(L, TimerMeta);
            timer->elapsed = 0.0f;
            timer->running = true;
            return 0;
        }

        int TimerIsRunning(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<TimerComponent>(L, TimerMeta)->running);
            return 1;
        }

        int TimerGetInterval(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<TimerComponent>(L, TimerMeta)->interval);
            return 1;
        }

        int TimerSetInterval(lua_State *L)
        {
            TimerComponent *timer = CheckComponent<TimerComponent>(L, TimerMeta);
            timer->interval = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int TimerIsRepeating(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<TimerComponent>(L, TimerMeta)->repeat);
            return 1;
        }

        int TimerSetRepeat(lua_State *L)
        {
            TimerComponent *timer = CheckComponent<TimerComponent>(L, TimerMeta);
            luaL_checkany(L, 2);
            timer->repeat = lua_toboolean(L, 2) != 0;
            return 0;
        }

        int TimerGetElapsed(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<TimerComponent>(L, TimerMeta)->elapsed);
            return 1;
        }

        int TimerGetRemaining(lua_State *L)
        {
            const TimerComponent *timer = CheckComponent<TimerComponent>(L, TimerMeta);
            const float remaining = timer->interval - timer->elapsed;
            lua_pushnumber(L, remaining > 0.0f ? remaining : 0.0f);
            return 1;
        }

        int PathPlay(lua_State *L)
        {
            CheckComponent<PathFollowerComponent>(L, PathMeta)->playing = true;
            return 0;
        }

        int PathPause(lua_State *L)
        {
            CheckComponent<PathFollowerComponent>(L, PathMeta)->playing = false;
            return 0;
        }

        int PathRestart(lua_State *L)
        {
            PathFollowerSystem::Restart(*CheckComponent<PathFollowerComponent>(L, PathMeta));
            return 0;
        }

        int PathIsPlaying(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<PathFollowerComponent>(L, PathMeta)->playing);
            return 1;
        }

        int PathGetSpeed(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<PathFollowerComponent>(L, PathMeta)->speed);
            return 1;
        }

        int PathSetSpeed(lua_State *L)
        {
            PathFollowerComponent *path = CheckComponent<PathFollowerComponent>(L, PathMeta);
            path->speed = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int PathAddPoint(lua_State *L)
        {
            PathFollowerComponent *path = CheckComponent<PathFollowerComponent>(L, PathMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            path->points.emplace_back(x, y);
            return 0;
        }

        int PathClearPoints(lua_State *L)
        {
            PathFollowerComponent *path = CheckComponent<PathFollowerComponent>(L, PathMeta);
            path->points.clear();
            path->target = 0;
            return 0;
        }

        int PathGetPointCount(lua_State *L)
        {
            lua_pushinteger(L, static_cast<lua_Integer>(CheckComponent<PathFollowerComponent>(L, PathMeta)->points.size()));
            return 1;
        }

        int PathGetPoint(lua_State *L)
        {
            const PathFollowerComponent *path = CheckComponent<PathFollowerComponent>(L, PathMeta);
            const lua_Integer index = luaL_checkinteger(L, 2);
            if (index < 1 || static_cast<size_t>(index) > path->points.size())
                return luaL_error(L, "path point %d is out of range", static_cast<int>(index));
            const glm::vec2 &point = path->points[static_cast<size_t>(index - 1)];
            lua_pushnumber(L, point.x);
            lua_pushnumber(L, point.y);
            return 2;
        }

        int PathSetPoint(lua_State *L)
        {
            PathFollowerComponent *path = CheckComponent<PathFollowerComponent>(L, PathMeta);
            const lua_Integer index = luaL_checkinteger(L, 2);
            const float x = static_cast<float>(luaL_checknumber(L, 3));
            const float y = static_cast<float>(luaL_checknumber(L, 4));
            if (index < 1 || static_cast<size_t>(index) > path->points.size())
                return luaL_error(L, "path point %d is out of range", static_cast<int>(index));
            path->points[static_cast<size_t>(index - 1)] = {x, y};
            return 0;
        }

        int HealthGetCurrent(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<HealthComponent>(L, HealthMeta)->current);
            return 1;
        }

        int HealthSetCurrent(lua_State *L)
        {
            HealthComponent *health = CheckComponent<HealthComponent>(L, HealthMeta);
            const float value = static_cast<float>(luaL_checknumber(L, 2));
            health->current = value < 0.0f ? 0.0f : (value > health->max ? health->max : value);
            if (health->current > 0.0f)
                health->dead = false;
            return 0;
        }

        int HealthGetMax(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<HealthComponent>(L, HealthMeta)->max);
            return 1;
        }

        int HealthSetMax(lua_State *L)
        {
            HealthComponent *health = CheckComponent<HealthComponent>(L, HealthMeta);
            const float value = static_cast<float>(luaL_checknumber(L, 2));
            health->max = value < 0.0f ? 0.0f : value;
            if (health->current > health->max)
                health->current = health->max;
            return 0;
        }

        int HealthGetFraction(lua_State *L)
        {
            const HealthComponent *health = CheckComponent<HealthComponent>(L, HealthMeta);
            lua_pushnumber(L, health->max > 0.0f ? health->current / health->max : 0.0f);
            return 1;
        }

        int HealthIsDead(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<HealthComponent>(L, HealthMeta)->dead);
            return 1;
        }

        int HealthIsInvulnerable(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<HealthComponent>(L, HealthMeta)->invulnerableRemaining > 0.0f);
            return 1;
        }

        int HealthGetInvulnerability(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<HealthComponent>(L, HealthMeta)->invulnerability);
            return 1;
        }

        int HealthSetInvulnerability(lua_State *L)
        {
            HealthComponent *health = CheckComponent<HealthComponent>(L, HealthMeta);
            const float value = static_cast<float>(luaL_checknumber(L, 2));
            health->invulnerability = value < 0.0f ? 0.0f : value;
            return 0;
        }

        int HealthDamage(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<HealthComponent>(L, HealthMeta);
            const float amount = static_cast<float>(luaL_checknumber(L, 2));
            entt::entity source = entt::null;
            if (!lua_isnoneornil(L, 3))
                source = CheckLiveEntity(L, 3)->GetHandle();
            lua_pushboolean(L, HealthSystem::Damage(*entity->GetScene(), entity->GetHandle(), amount, source));
            return 1;
        }

        int HealthHeal(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<HealthComponent>(L, HealthMeta);
            const float amount = static_cast<float>(luaL_checknumber(L, 2));
            lua_pushnumber(L, HealthSystem::Heal(*entity->GetScene(), entity->GetHandle(), amount));
            return 1;
        }

        int HealthKill(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<HealthComponent>(L, HealthMeta);
            entt::entity source = entt::null;
            if (!lua_isnoneornil(L, 2))
                source = CheckLiveEntity(L, 2)->GetHandle();
            lua_pushboolean(L, HealthSystem::Kill(*entity->GetScene(), entity->GetHandle(), source));
            return 1;
        }

        int HealthRevive(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<HealthComponent>(L, HealthMeta);
            lua_pushboolean(L, HealthSystem::Revive(*entity->GetScene(), entity->GetHandle()));
            return 1;
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

        int SceneSetFixedStep(lua_State *L)
        {
            Scene *scene = RequireScene(L);
            scene->SetFixedTimeStep(static_cast<float>(luaL_checknumber(L, 1)));
            return 0;
        }

        int AudioPlay(lua_State *L)
        {
            size_t length = 0;
            const char *path = luaL_checklstring(L, 1, &length);
            const float volume = static_cast<float>(luaL_optnumber(L, 2, 1.0));
            const float pitch = static_cast<float>(luaL_optnumber(L, 3, 1.0));
            const bool loop = lua_toboolean(L, 4) != 0;
            const VoiceId voice = AudioEngine::Play(std::string(path, length), volume, pitch, loop);
            lua_pushinteger(L, static_cast<lua_Integer>(voice));
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

        struct ComponentBinding
        {
            const char *Name;
            const char *Meta;
            const luaL_Reg *Methods;
            bool (*Has)(Entity &);
            bool (*Add)(Entity &);
            void (*Remove)(Entity &);
        };

        template <typename T>
        bool HasOf(Entity &entity)
        {
            return entity.HasComponent<T>();
        }

        template <typename T>
        bool AddOf(Entity &entity)
        {
            entity.AddComponent<T>();
            return true;
        }

        template <typename T>
        void RemoveOf(Entity &entity)
        {
            entity.RemoveComponent<T>();
        }

        bool AddSprite(Entity &entity)
        {
            SpriteComponent &sprite = entity.AddComponent<SpriteComponent>();
            sprite.width = 64.0f;
            sprite.height = 64.0f;
            return true;
        }

        bool AddShape(Entity &entity)
        {
            RenderComponent &shape = entity.AddComponent<RenderComponent>();
            shape.width = 64.0f;
            shape.height = 64.0f;
            return true;
        }

        const luaL_Reg SpriteMethods[] = {
            {"GetTint", SpriteGetTint},
            {"SetTint", SpriteSetTint},
            {"GetFlip", SpriteGetFlip},
            {"SetFlip", SpriteSetFlip},
            {"GetSize", SpriteGetSize},
            {"SetSize", SpriteSetSize},
            {"SetRegion", SpriteSetRegion},
            {"ClearRegion", SpriteClearRegion},
            {nullptr, nullptr}};

        const luaL_Reg CameraMethods[] = {
            {"Shake", CameraShake},
            {"GetZoom", CameraGetZoom},
            {"SetZoom", CameraSetZoom},
            {"SetFollow", CameraSetFollow},
            {"IsPrimary", CameraIsPrimary},
            {nullptr, nullptr}};

        const luaL_Reg LightMethods[] = {
            {"GetColor", LightGetColor},
            {"SetColor", LightSetColor},
            {"GetIntensity", LightGetIntensity},
            {"SetIntensity", LightSetIntensity},
            {"GetRadius", LightGetRadius},
            {"SetRadius", LightSetRadius},
            {"IsEnabled", LightIsEnabled},
            {"SetEnabled", LightSetEnabled},
            {nullptr, nullptr}};

        const luaL_Reg VelocityMethods[] = {
            {"Get", VelocityGet},
            {"Set", VelocitySet},
            {"Add", VelocityAdd},
            {nullptr, nullptr}};

        const luaL_Reg CollisionMethods[] = {
            {"GetSize", CollisionGetSize},
            {"SetSize", CollisionSetSize},
            {"GetOffset", CollisionGetOffset},
            {"SetOffset", CollisionSetOffset},
            {"GetBounds", CollisionGetBounds},
            {nullptr, nullptr}};

        const luaL_Reg ShapeMethods[] = {
            {"GetColor", ShapeGetColor},
            {"SetColor", ShapeSetColor},
            {"GetSize", ShapeGetSize},
            {"SetSize", ShapeSetSize},
            {nullptr, nullptr}};

        const luaL_Reg TextMethods[] = {
            {"GetText", TextGetText},
            {"SetText", TextSetText},
            {"GetFontSize", TextGetFontSize},
            {"SetFontSize", TextSetFontSize},
            {"GetColor", TextGetColor},
            {"SetColor", TextSetColor},
            {"GetAlign", TextGetAlign},
            {"SetAlign", TextSetAlign},
            {nullptr, nullptr}};

        const luaL_Reg GlowMethods[] = {
            {"GetColor", GlowGetColor},
            {"SetColor", GlowSetColor},
            {"GetIntensity", GlowGetIntensity},
            {"SetIntensity", GlowSetIntensity},
            {"GetSpread", GlowGetSpread},
            {"SetSpread", GlowSetSpread},
            {"IsEnabled", GlowIsEnabled},
            {"SetEnabled", GlowSetEnabled},
            {nullptr, nullptr}};

        const luaL_Reg ParticlesMethods[] = {
            {"Play", ParticlesPlay},
            {"Stop", ParticlesStop},
            {"IsPlaying", ParticlesIsPlaying},
            {"Restart", ParticlesRestart},
            {"Burst", ParticlesBurst},
            {"GetRate", ParticlesGetRate},
            {"SetRate", ParticlesSetRate},
            {nullptr, nullptr}};

        const luaL_Reg AudioMethods[] = {
            {"Play", AudioSourcePlay},
            {"Stop", AudioSourceStop},
            {"IsPlaying", AudioSourceIsPlaying},
            {"GetVolume", AudioSourceGetVolume},
            {"SetVolume", AudioSourceSetVolume},
            {"GetPitch", AudioSourceGetPitch},
            {"SetPitch", AudioSourceSetPitch},
            {"SetLoop", AudioSourceSetLoop},
            {"SetClip", AudioSourceSetClip},
            {nullptr, nullptr}};

        const luaL_Reg ParallaxMethods[] = {
            {"GetFactor", ParallaxGetFactor},
            {"SetFactor", ParallaxSetFactor},
            {nullptr, nullptr}};

        const luaL_Reg BodyMethods[] = {
            {"GetVelocity", BodyGetVelocity},
            {"SetVelocity", BodySetVelocity},
            {"AddImpulse", BodyAddImpulse},
            {"GetSpin", BodyGetSpin},
            {"SetSpin", BodySetSpin},
            {"IsStatic", BodyIsStatic},
            {"GetMass", BodyGetMass},
            {nullptr, nullptr}};

        const luaL_Reg TriggerMethods[] = {
            {"GetSize", TriggerGetSize},
            {"SetSize", TriggerSetSize},
            {"GetOffset", TriggerGetOffset},
            {"SetOffset", TriggerSetOffset},
            {"GetFilter", TriggerGetFilter},
            {"SetFilter", TriggerSetFilter},
            {"IsEnabled", TriggerIsEnabled},
            {"SetEnabled", TriggerSetEnabled},
            {"GetInside", TriggerGetInside},
            {"Contains", TriggerContains},
            {nullptr, nullptr}};

        const luaL_Reg TimerMethods[] = {
            {"Start", TimerStart},
            {"Stop", TimerStop},
            {"Restart", TimerRestart},
            {"IsRunning", TimerIsRunning},
            {"GetInterval", TimerGetInterval},
            {"SetInterval", TimerSetInterval},
            {"IsRepeating", TimerIsRepeating},
            {"SetRepeat", TimerSetRepeat},
            {"GetElapsed", TimerGetElapsed},
            {"GetRemaining", TimerGetRemaining},
            {nullptr, nullptr}};

        const luaL_Reg PathMethods[] = {
            {"Play", PathPlay},
            {"Pause", PathPause},
            {"Restart", PathRestart},
            {"IsPlaying", PathIsPlaying},
            {"GetSpeed", PathGetSpeed},
            {"SetSpeed", PathSetSpeed},
            {"AddPoint", PathAddPoint},
            {"ClearPoints", PathClearPoints},
            {"GetPointCount", PathGetPointCount},
            {"GetPoint", PathGetPoint},
            {"SetPoint", PathSetPoint},
            {nullptr, nullptr}};

        const luaL_Reg HealthMethods[] = {
            {"GetCurrent", HealthGetCurrent},
            {"SetCurrent", HealthSetCurrent},
            {"GetMax", HealthGetMax},
            {"SetMax", HealthSetMax},
            {"GetFraction", HealthGetFraction},
            {"IsDead", HealthIsDead},
            {"IsInvulnerable", HealthIsInvulnerable},
            {"GetInvulnerability", HealthGetInvulnerability},
            {"SetInvulnerability", HealthSetInvulnerability},
            {"Damage", HealthDamage},
            {"Heal", HealthHeal},
            {"Kill", HealthKill},
            {"Revive", HealthRevive},
            {nullptr, nullptr}};

        const ComponentBinding Bindings[] = {
            {"Sprite", SpriteMeta, SpriteMethods, &HasOf<SpriteComponent>, &AddSprite, &RemoveOf<SpriteComponent>},
            {"Camera", CameraMeta, CameraMethods, &HasOf<CameraComponent>, &AddOf<CameraComponent>, &RemoveOf<CameraComponent>},
            {"Light", LightMeta, LightMethods, &HasOf<LightComponent>, &AddOf<LightComponent>, &RemoveOf<LightComponent>},
            {"Velocity", VelocityMeta, VelocityMethods, &HasOf<VelocityComponent>, &AddOf<VelocityComponent>, &RemoveOf<VelocityComponent>},
            {"Collision", CollisionMeta, CollisionMethods, &HasOf<CollisionComponent>, &AddOf<CollisionComponent>, &RemoveOf<CollisionComponent>},
            {"Shape", ShapeMeta, ShapeMethods, &HasOf<RenderComponent>, &AddShape, &RemoveOf<RenderComponent>},
            {"Text", TextMeta, TextMethods, &HasOf<TextComponent>, &AddOf<TextComponent>, &RemoveOf<TextComponent>},
            {"Glow", GlowMeta, GlowMethods, &HasOf<GlowComponent>, &AddOf<GlowComponent>, &RemoveOf<GlowComponent>},
            {"Particles", ParticlesMeta, ParticlesMethods, &HasOf<ParticleEmitterComponent>, &AddOf<ParticleEmitterComponent>, &RemoveOf<ParticleEmitterComponent>},
            {"Audio", AudioMeta, AudioMethods, &HasOf<AudioSourceComponent>, &AddOf<AudioSourceComponent>, &RemoveOf<AudioSourceComponent>},
            {"Parallax", ParallaxMeta, ParallaxMethods, &HasOf<ParallaxComponent>, &AddOf<ParallaxComponent>, &RemoveOf<ParallaxComponent>},
            {"Body", BodyMeta, BodyMethods, &HasOf<PhysicsComponent>, nullptr, &RemoveOf<PhysicsComponent>},
            {"Trigger", TriggerMeta, TriggerMethods, &HasOf<TriggerComponent>, &AddOf<TriggerComponent>, &RemoveOf<TriggerComponent>},
            {"Timer", TimerMeta, TimerMethods, &HasOf<TimerComponent>, &AddOf<TimerComponent>, &RemoveOf<TimerComponent>},
            {"Path", PathMeta, PathMethods, &HasOf<PathFollowerComponent>, &AddOf<PathFollowerComponent>, &RemoveOf<PathFollowerComponent>},
            {"Health", HealthMeta, HealthMethods, &HasOf<HealthComponent>, &AddOf<HealthComponent>, &RemoveOf<HealthComponent>}};

        constexpr size_t BindingCount = sizeof(Bindings) / sizeof(Bindings[0]);

        const ComponentBinding *FindBinding(const char *name)
        {
            for (const ComponentBinding &binding : Bindings)
            {
                if (std::string_view(binding.Name) == name)
                    return &binding;
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
            const ComponentBinding &binding = Bindings[static_cast<size_t>(index)];
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
            {"Find", SceneFind},
            {"FindAll", SceneFindAll},
            {"SetAmbient", SceneSetAmbient},
            {"SetLighting", SceneSetLighting},
            {"SetFixedStep", SceneSetFixedStep},
            {nullptr, nullptr}};

        const luaL_Reg AudioFunctions[] = {
            {"Play", AudioPlay},
            {"Stop", AudioStop},
            {"StopAll", AudioStopAll},
            {"IsPlaying", AudioIsPlaying},
            {"SetVolume", AudioSetVolume},
            {"SetMasterVolume", AudioSetMasterVolume},
            {"GetMasterVolume", AudioGetMasterVolume},
            {nullptr, nullptr}};

        const luaL_Reg MathFunctions[] = {
            {"Lerp", MathLerp},
            {"Clamp", MathClamp},
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
            for (size_t i = 0; i < BindingCount; ++i)
            {
                const std::string method = std::string("Get") + Bindings[i].Name;
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

        luaL_newlib(L, MathFunctions);
        lua_setglobal(L, "Math");

        lua_pushcfunction(L, LogInfo);
        lua_setglobal(L, "print");

        RegisterKeys(L);
        RegisterEntity(L);
        for (size_t i = 0; i < BindingCount; ++i)
            RegisterHandle(L, Bindings[i].Meta, Bindings[i].Methods);
    }
}
