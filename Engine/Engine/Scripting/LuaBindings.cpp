#include "LuaBindings.h"
#include "LuaInclude.h"

#include <Engine/Core/Log.h>
#include <Engine/Input/Input.h>
#include <Engine/Input/SIMP_Keys.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>

#include <new>
#include <string_view>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *EntityMeta = "SIMP.Entity";
        constexpr const char *SpriteMeta = "SIMP.Sprite";
        constexpr const char *CameraMeta = "SIMP.Camera";
        constexpr const char *LightMeta = "SIMP.Light";

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

        TransformComponent *CheckTransform(lua_State *L, int index)
        {
            Entity *entity = CheckLiveEntity(L, index);
            if (!entity->HasComponent<TransformComponent>())
                luaL_error(L, "entity has no transform");
            return &entity->GetComponent<TransformComponent>();
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

        int EntitySetPosition(lua_State *L)
        {
            TransformComponent *transform = CheckTransform(L, 1);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            transform->position = {x, y};
            transform->dirty = true;
            return 0;
        }

        int EntityTranslate(lua_State *L)
        {
            TransformComponent *transform = CheckTransform(L, 1);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            transform->position += glm::vec2(x, y);
            transform->dirty = true;
            return 0;
        }

        int EntityGetRotation(lua_State *L)
        {
            lua_pushnumber(L, CheckTransform(L, 1)->rotation);
            return 1;
        }

        int EntitySetRotation(lua_State *L)
        {
            TransformComponent *transform = CheckTransform(L, 1);
            transform->rotation = static_cast<float>(luaL_checknumber(L, 2));
            transform->dirty = true;
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
            const std::string_view wanted(name, length);

            Scene *scene = entity->GetScene();
            auto view = scene->GetRegistry().view<TagComponent>();
            for (auto handle : view)
            {
                if (view.get<TagComponent>(handle).Tag == wanted)
                {
                    PushEntity(L, Entity(handle, scene));
                    return 1;
                }
            }

            lua_pushnil(L);
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

        template <typename T>
        T *CheckComponent(lua_State *L, const char *meta)
        {
            Entity *entity = static_cast<Entity *>(luaL_checkudata(L, 1, meta));
            if (!static_cast<bool>(*entity))
                luaL_error(L, "entity is no longer valid");
            if (!entity->HasComponent<T>())
                luaL_error(L, "entity no longer has this component");
            return &entity->GetComponent<T>();
        }

        template <typename T>
        int PushComponentHandle(lua_State *L, const char *meta)
        {
            Entity *entity = CheckLiveEntity(L, 1);
            if (!entity->HasComponent<T>())
            {
                lua_pushnil(L);
                return 1;
            }

            void *memory = lua_newuserdatauv(L, sizeof(Entity), 0);
            new (memory) Entity(*entity);
            luaL_setmetatable(L, meta);
            return 1;
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

        int EntityGetSprite(lua_State *L) { return PushComponentHandle<SpriteComponent>(L, SpriteMeta); }
        int EntityGetCamera(lua_State *L) { return PushComponentHandle<CameraComponent>(L, CameraMeta); }
        int EntityGetLight(lua_State *L) { return PushComponentHandle<LightComponent>(L, LightMeta); }

        int SpriteGetTint(lua_State *L)
        {
            const SpriteComponent *sprite = CheckComponent<SpriteComponent>(L, SpriteMeta);
            lua_pushinteger(L, sprite->tint.r);
            lua_pushinteger(L, sprite->tint.g);
            lua_pushinteger(L, sprite->tint.b);
            lua_pushinteger(L, sprite->tint.a);
            return 4;
        }

        int SpriteSetTint(lua_State *L)
        {
            SpriteComponent *sprite = CheckComponent<SpriteComponent>(L, SpriteMeta);
            const Uint8 r = ToByte(L, 2);
            const Uint8 g = ToByte(L, 3);
            const Uint8 b = ToByte(L, 4);
            const Uint8 a = ToByteOpt(L, 5, 255);
            sprite->tint = SDL_Color{r, g, b, a};
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
            {nullptr, nullptr}};

        const luaL_Reg EntityMethods[] = {
            {"IsValid", EntityIsValid},
            {"GetName", EntityGetName},
            {"SetName", EntitySetName},
            {"GetPosition", EntityGetPosition},
            {"SetPosition", EntitySetPosition},
            {"Translate", EntityTranslate},
            {"GetRotation", EntityGetRotation},
            {"SetRotation", EntitySetRotation},
            {"GetScale", EntityGetScale},
            {"SetScale", EntitySetScale},
            {"GetMouseWorld", EntityGetMouseWorld},
            {"FindByName", EntityFindByName},
            {"Destroy", EntityDestroy},
            {"GetSprite", EntityGetSprite},
            {"GetCamera", EntityGetCamera},
            {"GetLight", EntityGetLight},
            {nullptr, nullptr}};

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
            lua_setfield(L, -2, "__index");

            lua_pushcfunction(L, EntityEquals);
            lua_setfield(L, -2, "__eq");

            lua_pushcfunction(L, EntityToString);
            lua_setfield(L, -2, "__tostring");

            lua_pop(L, 1);
        }
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

        lua_pushcfunction(L, LogInfo);
        lua_setglobal(L, "print");

        RegisterKeys(L);
        RegisterEntity(L);
        RegisterHandle(L, SpriteMeta, SpriteMethods);
        RegisterHandle(L, CameraMeta, CameraMethods);
        RegisterHandle(L, LightMeta, LightMethods);
    }
}
