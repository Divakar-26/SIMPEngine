#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *ShapeMeta = "SIMP.Shape";

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

        bool AddShape(Entity &entity)
        {
            RenderComponent &shape = entity.AddComponent<RenderComponent>();
            shape.width = 64.0f;
            shape.height = 64.0f;
            return true;
        }

        const luaL_Reg ShapeMethods[] = {
            {"GetColor", ShapeGetColor},
            {"SetColor", ShapeSetColor},
            {"GetSize", ShapeGetSize},
            {"SetSize", ShapeSetSize},
            {nullptr, nullptr}};
    }

    const ComponentBinding &ShapeBinding()
    {
        static const ComponentBinding binding{"Shape", ShapeMeta, ShapeMethods, &HasOf<RenderComponent>, &AddShape, &RemoveOf<RenderComponent>};
        return binding;
    }
}
