#include "LuaComponents.h"

#include <Engine/Core/GameVariables.h>
#include <Engine/Rendering/Font.h>
#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *TextMeta = "SIMP.Text";

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

        int TextSetFont(lua_State *L)
        {
            TextComponent *text = CheckComponent<TextComponent>(L, TextMeta);
            size_t length = 0;
            const char *path = luaL_optlstring(L, 2, "", &length);
            text->FontPath.assign(path, length);
            return 0;
        }

        int TextGetFont(lua_State *L)
        {
            const TextComponent *text = CheckComponent<TextComponent>(L, TextMeta);
            lua_pushlstring(L, text->FontPath.data(), text->FontPath.size());
            return 1;
        }

        int TextMeasure(lua_State *L)
        {
            const TextComponent *text = CheckComponent<TextComponent>(L, TextMeta);
            glm::vec2 extent{0.0f};
            {
                auto font = text->FontPath.empty() ? Font::GetDefault() : Font::Load(text->FontPath);
                if (font)
                    extent = font->MeasureText(GameVariables::Format(text->Text), text->Size);
            }
            lua_pushnumber(L, extent.x);
            lua_pushnumber(L, extent.y);
            return 2;
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

        const luaL_Reg TextMethods[] = {
            {"GetText", TextGetText},
            {"SetText", TextSetText},
            {"GetFontSize", TextGetFontSize},
            {"SetFontSize", TextSetFontSize},
            {"GetColor", TextGetColor},
            {"SetColor", TextSetColor},
            {"SetFont", TextSetFont},
            {"GetFont", TextGetFont},
            {"Measure", TextMeasure},
            {"GetAlign", TextGetAlign},
            {"SetAlign", TextSetAlign},
            {nullptr, nullptr}};
    }

    const ComponentBinding &TextBinding()
    {
        static const ComponentBinding binding{"Text", TextMeta, TextMethods, &HasOf<TextComponent>, &AddOf<TextComponent>, &RemoveOf<TextComponent>};
        return binding;
    }
}
