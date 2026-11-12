#include "LuaComponents.h"

#include <Engine/Rendering/TextureManager.h>
#include <Engine/Scene/Systems/ShaderCanvasSystem.h>
#include <Engine/Scripting/LuaHelpers.h>

#include <algorithm>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *ShaderMeta = "SIMP.ShaderCanvas";

        ShaderUniformValue &Ensure(ShaderCanvasComponent &canvas, const char *name, int type)
        {
            if (ShaderUniformValue *existing = ShaderCanvasSystem::FindUniform(canvas, name))
                return *existing;
            ShaderUniformValue value;
            value.name = name;
            value.type = type;
            canvas.uniforms.push_back(std::move(value));
            return canvas.uniforms.back();
        }

        float Num(lua_State *L, int index)
        {
            return static_cast<float>(luaL_checknumber(L, index));
        }

        int ShaderSetShader(lua_State *L)
        {
            ShaderCanvasComponent *c = CheckComponent<ShaderCanvasComponent>(L, ShaderMeta);
            size_t length = 0;
            const char *text = luaL_optlstring(L, 2, "", &length);
            c->shader.assign(text, length);
            return 0;
        }
        int ShaderGetShader(lua_State *L)
        {
            const ShaderCanvasComponent *c = CheckComponent<ShaderCanvasComponent>(L, ShaderMeta);
            lua_pushlstring(L, c->shader.data(), c->shader.size());
            return 1;
        }
        int ShaderSetEnabled(lua_State *L)
        {
            ShaderCanvasComponent *c = CheckComponent<ShaderCanvasComponent>(L, ShaderMeta);
            luaL_checkany(L, 2);
            c->enabled = lua_toboolean(L, 2) != 0;
            return 0;
        }
        int ShaderIsEnabled(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<ShaderCanvasComponent>(L, ShaderMeta)->enabled);
            return 1;
        }
        int ShaderSetProgress(lua_State *L)
        {
            ShaderCanvasComponent *c = CheckComponent<ShaderCanvasComponent>(L, ShaderMeta);
            c->progress = Num(L, 2);
            return 0;
        }
        int ShaderGetProgress(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<ShaderCanvasComponent>(L, ShaderMeta)->progress);
            return 1;
        }
        int ShaderGetError(lua_State *L)
        {
            const ShaderCanvasComponent *c = CheckComponent<ShaderCanvasComponent>(L, ShaderMeta);
            lua_pushlstring(L, c->error.data(), c->error.size());
            return 1;
        }
        int ShaderSetFloat(lua_State *L)
        {
            ShaderCanvasComponent *c = CheckComponent<ShaderCanvasComponent>(L, ShaderMeta);
            const char *name = luaL_checkstring(L, 2);
            const float value = Num(L, 3);
            Ensure(*c, name, 0).value[0] = value;
            return 0;
        }
        int ShaderGetFloat(lua_State *L)
        {
            ShaderCanvasComponent *c = CheckComponent<ShaderCanvasComponent>(L, ShaderMeta);
            const char *name = luaL_checkstring(L, 2);
            const ShaderUniformValue *value = ShaderCanvasSystem::FindUniform(*c, name);
            lua_pushnumber(L, value ? value->value[0] : 0.0f);
            return 1;
        }
        int ShaderSetVec2(lua_State *L)
        {
            ShaderCanvasComponent *c = CheckComponent<ShaderCanvasComponent>(L, ShaderMeta);
            const char *name = luaL_checkstring(L, 2);
            const float x = Num(L, 3), y = Num(L, 4);
            ShaderUniformValue &value = Ensure(*c, name, 1);
            value.value[0] = x;
            value.value[1] = y;
            return 0;
        }
        int ShaderSetVec3(lua_State *L)
        {
            ShaderCanvasComponent *c = CheckComponent<ShaderCanvasComponent>(L, ShaderMeta);
            const char *name = luaL_checkstring(L, 2);
            const float x = Num(L, 3), y = Num(L, 4), z = Num(L, 5);
            ShaderUniformValue &value = Ensure(*c, name, 2);
            value.value[0] = x;
            value.value[1] = y;
            value.value[2] = z;
            return 0;
        }
        int ShaderSetVec4(lua_State *L)
        {
            ShaderCanvasComponent *c = CheckComponent<ShaderCanvasComponent>(L, ShaderMeta);
            const char *name = luaL_checkstring(L, 2);
            const float x = Num(L, 3), y = Num(L, 4), z = Num(L, 5), w = Num(L, 6);
            ShaderUniformValue &value = Ensure(*c, name, 3);
            value.value[0] = x;
            value.value[1] = y;
            value.value[2] = z;
            value.value[3] = w;
            return 0;
        }
        int ShaderSetColor(lua_State *L)
        {
            ShaderCanvasComponent *c = CheckComponent<ShaderCanvasComponent>(L, ShaderMeta);
            const char *name = luaL_checkstring(L, 2);
            SDL_Color color{255, 255, 255, 255};
            ReadRgba(L, 3, color);
            ShaderUniformValue &value = Ensure(*c, name, 4);
            value.value[0] = color.r / 255.0f;
            value.value[1] = color.g / 255.0f;
            value.value[2] = color.b / 255.0f;
            value.value[3] = color.a / 255.0f;
            return 0;
        }
        int ShaderSetInt(lua_State *L)
        {
            ShaderCanvasComponent *c = CheckComponent<ShaderCanvasComponent>(L, ShaderMeta);
            const char *name = luaL_checkstring(L, 2);
            const float value = static_cast<float>(luaL_checkinteger(L, 3));
            Ensure(*c, name, 6).value[0] = value;
            return 0;
        }
        int ShaderSetBool(lua_State *L)
        {
            ShaderCanvasComponent *c = CheckComponent<ShaderCanvasComponent>(L, ShaderMeta);
            const char *name = luaL_checkstring(L, 2);
            luaL_checkany(L, 3);
            const float value = lua_toboolean(L, 3) ? 1.0f : 0.0f;
            Ensure(*c, name, 7).value[0] = value;
            return 0;
        }
        int ShaderSetTexture(lua_State *L)
        {
            ShaderCanvasComponent *c = CheckComponent<ShaderCanvasComponent>(L, ShaderMeta);
            const char *name = luaL_checkstring(L, 2);
            size_t length = 0;
            const char *text = luaL_checklstring(L, 3, &length);
            const std::string path(text, length);
            ShaderUniformValue &value = Ensure(*c, name, 5);
            value.texturePath = path;
            value.texture = path.empty() ? nullptr : TextureManager::Get().LoadTexture(path, path);
            return 0;
        }
        int ShaderHasUniform(lua_State *L)
        {
            ShaderCanvasComponent *c = CheckComponent<ShaderCanvasComponent>(L, ShaderMeta);
            lua_pushboolean(L, ShaderCanvasSystem::FindUniform(*c, luaL_checkstring(L, 2)) != nullptr);
            return 1;
        }

        const luaL_Reg ShaderMethods[] = {
            {"SetShader", ShaderSetShader},
            {"GetShader", ShaderGetShader},
            {"SetEnabled", ShaderSetEnabled},
            {"IsEnabled", ShaderIsEnabled},
            {"SetProgress", ShaderSetProgress},
            {"GetProgress", ShaderGetProgress},
            {"GetError", ShaderGetError},
            {"SetFloat", ShaderSetFloat},
            {"GetFloat", ShaderGetFloat},
            {"SetVec2", ShaderSetVec2},
            {"SetVec3", ShaderSetVec3},
            {"SetVec4", ShaderSetVec4},
            {"SetColor", ShaderSetColor},
            {"SetInt", ShaderSetInt},
            {"SetBool", ShaderSetBool},
            {"SetTexture", ShaderSetTexture},
            {"HasUniform", ShaderHasUniform},
            {nullptr, nullptr}};
    }

    const ComponentBinding &ShaderCanvasBinding()
    {
        static const ComponentBinding binding{"ShaderCanvas", ShaderMeta, ShaderMethods, &HasOf<ShaderCanvasComponent>, &AddOf<ShaderCanvasComponent>, &RemoveOf<ShaderCanvasComponent>};
        return binding;
    }
}
