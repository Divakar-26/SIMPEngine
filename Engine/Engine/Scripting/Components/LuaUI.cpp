#include "LuaComponents.h"

#include <Engine/Rendering/TextureManager.h>
#include <Engine/Scripting/LuaHelpers.h>

#include <algorithm>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *CanvasMeta = "SIMP.Canvas";
        constexpr const char *RectMeta = "SIMP.UIRect";
        constexpr const char *PanelMeta = "SIMP.UIPanel";
        constexpr const char *LabelMeta = "SIMP.UILabel";
        constexpr const char *ImageMeta = "SIMP.UIImage";
        constexpr const char *ButtonMeta = "SIMP.UIButton";
        constexpr const char *LayoutMeta = "SIMP.UILayout";

        float Num(lua_State *L, int index)
        {
            return static_cast<float>(luaL_checknumber(L, index));
        }

        int CanvasSetVisible(lua_State *L)
        {
            CanvasComponent *c = CheckComponent<CanvasComponent>(L, CanvasMeta);
            luaL_checkany(L, 2);
            c->visible = lua_toboolean(L, 2) != 0;
            return 0;
        }
        int CanvasIsVisible(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<CanvasComponent>(L, CanvasMeta)->visible);
            return 1;
        }
        int CanvasGetSize(lua_State *L)
        {
            const CanvasComponent *c = CheckComponent<CanvasComponent>(L, CanvasMeta);
            lua_pushnumber(L, c->size.x);
            lua_pushnumber(L, c->size.y);
            return 2;
        }
        int CanvasGetScale(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<CanvasComponent>(L, CanvasMeta)->scale);
            return 1;
        }
        int CanvasSetReferenceSize(lua_State *L)
        {
            CanvasComponent *c = CheckComponent<CanvasComponent>(L, CanvasMeta);
            c->referenceSize = {Num(L, 2), Num(L, 3)};
            return 0;
        }
        int CanvasSetSortOrder(lua_State *L)
        {
            CanvasComponent *c = CheckComponent<CanvasComponent>(L, CanvasMeta);
            c->sortOrder = static_cast<int>(luaL_checkinteger(L, 2));
            return 0;
        }

        const luaL_Reg CanvasMethods[] = {
            {"SetVisible", CanvasSetVisible},
            {"IsVisible", CanvasIsVisible},
            {"GetSize", CanvasGetSize},
            {"GetScale", CanvasGetScale},
            {"SetReferenceSize", CanvasSetReferenceSize},
            {"SetSortOrder", CanvasSetSortOrder},
            {nullptr, nullptr}};

        int RectSetPosition(lua_State *L)
        {
            UIRectComponent *c = CheckComponent<UIRectComponent>(L, RectMeta);
            c->position = {Num(L, 2), Num(L, 3)};
            return 0;
        }
        int RectGetPosition(lua_State *L)
        {
            const UIRectComponent *c = CheckComponent<UIRectComponent>(L, RectMeta);
            lua_pushnumber(L, c->position.x);
            lua_pushnumber(L, c->position.y);
            return 2;
        }
        int RectSetSize(lua_State *L)
        {
            UIRectComponent *c = CheckComponent<UIRectComponent>(L, RectMeta);
            c->size = {Num(L, 2), Num(L, 3)};
            return 0;
        }
        int RectGetSize(lua_State *L)
        {
            const UIRectComponent *c = CheckComponent<UIRectComponent>(L, RectMeta);
            lua_pushnumber(L, c->size.x);
            lua_pushnumber(L, c->size.y);
            return 2;
        }
        int RectSetAnchors(lua_State *L)
        {
            UIRectComponent *c = CheckComponent<UIRectComponent>(L, RectMeta);
            const float a = Num(L, 2), b = Num(L, 3);
            const float x = static_cast<float>(luaL_optnumber(L, 4, a));
            const float y = static_cast<float>(luaL_optnumber(L, 5, b));
            c->anchorMin = {a, b};
            c->anchorMax = {x, y};
            return 0;
        }
        int RectSetPivot(lua_State *L)
        {
            UIRectComponent *c = CheckComponent<UIRectComponent>(L, RectMeta);
            c->pivot = {Num(L, 2), Num(L, 3)};
            return 0;
        }
        int RectSetMargin(lua_State *L)
        {
            UIRectComponent *c = CheckComponent<UIRectComponent>(L, RectMeta);
            c->margin = {Num(L, 2), Num(L, 3), Num(L, 4), Num(L, 5)};
            return 0;
        }
        int RectSetVisible(lua_State *L)
        {
            UIRectComponent *c = CheckComponent<UIRectComponent>(L, RectMeta);
            luaL_checkany(L, 2);
            c->visible = lua_toboolean(L, 2) != 0;
            return 0;
        }
        int RectIsVisible(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<UIRectComponent>(L, RectMeta)->visible);
            return 1;
        }
        int RectSetBlocksInput(lua_State *L)
        {
            UIRectComponent *c = CheckComponent<UIRectComponent>(L, RectMeta);
            luaL_checkany(L, 2);
            c->blocksInput = lua_toboolean(L, 2) != 0;
            return 0;
        }
        int RectGetRect(lua_State *L)
        {
            const UIRectComponent *c = CheckComponent<UIRectComponent>(L, RectMeta);
            lua_pushnumber(L, c->rect.x);
            lua_pushnumber(L, c->rect.y);
            lua_pushnumber(L, c->rect.z);
            lua_pushnumber(L, c->rect.w);
            return 4;
        }

        const luaL_Reg RectMethods[] = {
            {"SetPosition", RectSetPosition},
            {"GetPosition", RectGetPosition},
            {"SetSize", RectSetSize},
            {"GetSize", RectGetSize},
            {"SetAnchors", RectSetAnchors},
            {"SetPivot", RectSetPivot},
            {"SetMargin", RectSetMargin},
            {"SetVisible", RectSetVisible},
            {"IsVisible", RectIsVisible},
            {"SetBlocksInput", RectSetBlocksInput},
            {"GetRect", RectGetRect},
            {nullptr, nullptr}};

        int PanelSetColor(lua_State *L)
        {
            UIPanelComponent *c = CheckComponent<UIPanelComponent>(L, PanelMeta);
            ReadRgba(L, 2, c->color);
            return 0;
        }
        int PanelGetColor(lua_State *L)
        {
            return PushRgba(L, CheckComponent<UIPanelComponent>(L, PanelMeta)->color);
        }
        int PanelSetCornerRadius(lua_State *L)
        {
            UIPanelComponent *c = CheckComponent<UIPanelComponent>(L, PanelMeta);
            c->cornerRadius = std::max(0.0f, Num(L, 2));
            return 0;
        }
        int PanelSetBorder(lua_State *L)
        {
            UIPanelComponent *c = CheckComponent<UIPanelComponent>(L, PanelMeta);
            c->borderWidth = std::max(0.0f, Num(L, 2));
            if (!lua_isnoneornil(L, 3))
                ReadRgba(L, 3, c->borderColor);
            return 0;
        }

        const luaL_Reg PanelMethods[] = {
            {"SetColor", PanelSetColor},
            {"GetColor", PanelGetColor},
            {"SetCornerRadius", PanelSetCornerRadius},
            {"SetBorder", PanelSetBorder},
            {nullptr, nullptr}};

        int LabelGetText(lua_State *L)
        {
            const UILabelComponent *c = CheckComponent<UILabelComponent>(L, LabelMeta);
            lua_pushlstring(L, c->text.data(), c->text.size());
            return 1;
        }
        int LabelSetText(lua_State *L)
        {
            UILabelComponent *c = CheckComponent<UILabelComponent>(L, LabelMeta);
            size_t length = 0;
            const char *value = luaL_checklstring(L, 2, &length);
            c->text.assign(value, length);
            return 0;
        }
        int LabelGetFontSize(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<UILabelComponent>(L, LabelMeta)->size);
            return 1;
        }
        int LabelSetFontSize(lua_State *L)
        {
            UILabelComponent *c = CheckComponent<UILabelComponent>(L, LabelMeta);
            c->size = std::max(1.0f, Num(L, 2));
            return 0;
        }
        int LabelSetColor(lua_State *L)
        {
            UILabelComponent *c = CheckComponent<UILabelComponent>(L, LabelMeta);
            ReadRgba(L, 2, c->color);
            return 0;
        }
        int LabelGetColor(lua_State *L)
        {
            return PushRgba(L, CheckComponent<UILabelComponent>(L, LabelMeta)->color);
        }
        int LabelSetAlign(lua_State *L)
        {
            UILabelComponent *c = CheckComponent<UILabelComponent>(L, LabelMeta);
            const lua_Integer h = luaL_checkinteger(L, 2);
            const lua_Integer v = luaL_optinteger(L, 3, c->verticalAlign);
            c->align = static_cast<int>(std::clamp<lua_Integer>(h, 0, 2));
            c->verticalAlign = static_cast<int>(std::clamp<lua_Integer>(v, 0, 2));
            return 0;
        }
        int LabelSetShadow(lua_State *L)
        {
            UILabelComponent *c = CheckComponent<UILabelComponent>(L, LabelMeta);
            luaL_checkany(L, 2);
            c->shadow = lua_toboolean(L, 2) != 0;
            return 0;
        }

        const luaL_Reg LabelMethods[] = {
            {"GetText", LabelGetText},
            {"SetText", LabelSetText},
            {"GetFontSize", LabelGetFontSize},
            {"SetFontSize", LabelSetFontSize},
            {"SetColor", LabelSetColor},
            {"GetColor", LabelGetColor},
            {"SetAlign", LabelSetAlign},
            {"SetShadow", LabelSetShadow},
            {nullptr, nullptr}};

        int ImageSetTexture(lua_State *L)
        {
            UIImageComponent *c = CheckComponent<UIImageComponent>(L, ImageMeta);
            size_t length = 0;
            const char *value = luaL_checklstring(L, 2, &length);
            const std::string path(value, length);
            c->texture = path.empty() ? nullptr : TextureManager::Get().LoadTexture(path, path);
            return 0;
        }
        int ImageSetTint(lua_State *L)
        {
            UIImageComponent *c = CheckComponent<UIImageComponent>(L, ImageMeta);
            ReadRgba(L, 2, c->tint);
            return 0;
        }
        int ImageGetTint(lua_State *L)
        {
            return PushRgba(L, CheckComponent<UIImageComponent>(L, ImageMeta)->tint);
        }
        int ImageSetPreserveAspect(lua_State *L)
        {
            UIImageComponent *c = CheckComponent<UIImageComponent>(L, ImageMeta);
            luaL_checkany(L, 2);
            c->preserveAspect = lua_toboolean(L, 2) != 0;
            return 0;
        }

        const luaL_Reg ImageMethods[] = {
            {"SetTexture", ImageSetTexture},
            {"SetTint", ImageSetTint},
            {"GetTint", ImageGetTint},
            {"SetPreserveAspect", ImageSetPreserveAspect},
            {nullptr, nullptr}};

        int UIButtonSetEnabled(lua_State *L)
        {
            UIButtonComponent *c = CheckComponent<UIButtonComponent>(L, ButtonMeta);
            luaL_checkany(L, 2);
            c->enabled = lua_toboolean(L, 2) != 0;
            return 0;
        }
        int UIButtonIsEnabled(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<UIButtonComponent>(L, ButtonMeta)->enabled);
            return 1;
        }
        int UIButtonIsHovered(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<UIButtonComponent>(L, ButtonMeta)->hovering);
            return 1;
        }
        int UIButtonIsPressed(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<UIButtonComponent>(L, ButtonMeta)->pressed);
            return 1;
        }
        int UIButtonIsFocused(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<UIButtonComponent>(L, ButtonMeta)->focused);
            return 1;
        }
        int UIButtonSetHoverScale(lua_State *L)
        {
            UIButtonComponent *c = CheckComponent<UIButtonComponent>(L, ButtonMeta);
            c->hoverScale = Num(L, 2);
            return 0;
        }
        int UIButtonSetPressScale(lua_State *L)
        {
            UIButtonComponent *c = CheckComponent<UIButtonComponent>(L, ButtonMeta);
            c->pressScale = Num(L, 2);
            return 0;
        }
        int UIButtonSetClickSound(lua_State *L)
        {
            UIButtonComponent *c = CheckComponent<UIButtonComponent>(L, ButtonMeta);
            size_t length = 0;
            const char *text = luaL_optlstring(L, 2, "", &length);
            c->clickSound.assign(text, length);
            return 0;
        }
        int UIButtonSetHoverSound(lua_State *L)
        {
            UIButtonComponent *c = CheckComponent<UIButtonComponent>(L, ButtonMeta);
            size_t length = 0;
            const char *text = luaL_optlstring(L, 2, "", &length);
            c->hoverSound.assign(text, length);
            return 0;
        }

        const luaL_Reg ButtonMethods[] = {
            {"SetEnabled", UIButtonSetEnabled},
            {"IsEnabled", UIButtonIsEnabled},
            {"IsHovered", UIButtonIsHovered},
            {"IsPressed", UIButtonIsPressed},
            {"IsFocused", UIButtonIsFocused},
            {"SetHoverScale", UIButtonSetHoverScale},
            {"SetPressScale", UIButtonSetPressScale},
            {"SetClickSound", UIButtonSetClickSound},
            {"SetHoverSound", UIButtonSetHoverSound},
            {nullptr, nullptr}};

        int LayoutSetSpacing(lua_State *L)
        {
            UILayoutComponent *c = CheckComponent<UILayoutComponent>(L, LayoutMeta);
            c->spacing = Num(L, 2);
            return 0;
        }
        int LayoutSetPadding(lua_State *L)
        {
            UILayoutComponent *c = CheckComponent<UILayoutComponent>(L, LayoutMeta);
            const float left = Num(L, 2);
            const float bottom = static_cast<float>(luaL_optnumber(L, 3, left));
            const float right = static_cast<float>(luaL_optnumber(L, 4, left));
            const float top = static_cast<float>(luaL_optnumber(L, 5, bottom));
            c->padding = {left, bottom, right, top};
            return 0;
        }
        int LayoutSetDirection(lua_State *L)
        {
            UILayoutComponent *c = CheckComponent<UILayoutComponent>(L, LayoutMeta);
            const char *value = luaL_checkstring(L, 2);
            c->direction = value[0] == 'r' || value[0] == 'R' ? 0 : 1;
            return 0;
        }
        int LayoutSetEnabled(lua_State *L)
        {
            UILayoutComponent *c = CheckComponent<UILayoutComponent>(L, LayoutMeta);
            luaL_checkany(L, 2);
            c->enabled = lua_toboolean(L, 2) != 0;
            return 0;
        }

        const luaL_Reg LayoutMethods[] = {
            {"SetSpacing", LayoutSetSpacing},
            {"SetPadding", LayoutSetPadding},
            {"SetDirection", LayoutSetDirection},
            {"SetEnabled", LayoutSetEnabled},
            {nullptr, nullptr}};
    }

    const ComponentBinding &CanvasBinding()
    {
        static const ComponentBinding binding{"Canvas", CanvasMeta, CanvasMethods, &HasOf<CanvasComponent>, &AddOf<CanvasComponent>, &RemoveOf<CanvasComponent>};
        return binding;
    }

    const ComponentBinding &UIRectBinding()
    {
        static const ComponentBinding binding{"UIRect", RectMeta, RectMethods, &HasOf<UIRectComponent>, &AddOf<UIRectComponent>, &RemoveOf<UIRectComponent>};
        return binding;
    }

    const ComponentBinding &UIPanelBinding()
    {
        static const ComponentBinding binding{"UIPanel", PanelMeta, PanelMethods, &HasOf<UIPanelComponent>, &AddOf<UIPanelComponent>, &RemoveOf<UIPanelComponent>};
        return binding;
    }

    const ComponentBinding &UILabelBinding()
    {
        static const ComponentBinding binding{"UILabel", LabelMeta, LabelMethods, &HasOf<UILabelComponent>, &AddOf<UILabelComponent>, &RemoveOf<UILabelComponent>};
        return binding;
    }

    const ComponentBinding &UIImageBinding()
    {
        static const ComponentBinding binding{"UIImage", ImageMeta, ImageMethods, &HasOf<UIImageComponent>, &AddOf<UIImageComponent>, &RemoveOf<UIImageComponent>};
        return binding;
    }

    const ComponentBinding &UIButtonBinding()
    {
        static const ComponentBinding binding{"UIButton", ButtonMeta, ButtonMethods, &HasOf<UIButtonComponent>, &AddOf<UIButtonComponent>, &RemoveOf<UIButtonComponent>};
        return binding;
    }

    const ComponentBinding &UILayoutBinding()
    {
        static const ComponentBinding binding{"UILayout", LayoutMeta, LayoutMethods, &HasOf<UILayoutComponent>, &AddOf<UILayoutComponent>, &RemoveOf<UILayoutComponent>};
        return binding;
    }
}
