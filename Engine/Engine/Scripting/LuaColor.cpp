#include "LuaModules.h"

#include "LuaHelpers.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <string>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        std::map<std::string, SDL_Color> &Palette()
        {
            static std::map<std::string, SDL_Color> palette;
            return palette;
        }

        void ResetPalette()
        {
            auto &p = Palette();
            p.clear();
            auto add = [&p](const char *name, Uint8 r, Uint8 g, Uint8 b)
            { p[name] = SDL_Color{r, g, b, 255}; };
            add("white", 255, 255, 255);
            add("black", 0, 0, 0);
            add("bg", 14, 17, 30);
            add("panel", 25, 30, 52);
            add("ink", 236, 241, 252);
            add("muted", 138, 148, 176);
            add("faint", 62, 71, 100);
            add("cyan", 88, 206, 255);
            add("teal", 62, 214, 176);
            add("coral", 255, 126, 96);
            add("gold", 255, 206, 96);
            add("violet", 172, 142, 255);
            add("pink", 255, 126, 182);
            add("red", 255, 93, 93);
            add("orange", 255, 162, 77);
            add("green", 95, 211, 141);
            add("blue", 90, 160, 255);
        }
    }

    bool LookupColorName(const char *name, SDL_Color &out)
    {
        if (!name)
            return false;
        const auto &p = Palette();
        const auto it = p.find(name);
        if (it == p.end())
            return false;
        out = it->second;
        return true;
    }

    const char *ColorNameList()
    {
        static std::string list;
        list.clear();
        for (const auto &[name, color] : Palette())
            list += (list.empty() ? "" : ", ") + name;
        return list.c_str();
    }

    bool ParseHexColor(const char *text, Uint8 out[4])
    {
        if (!text)
            return false;
        if (*text == '#')
            ++text;
        const size_t length = std::strlen(text);
        if (length != 3 && length != 4 && length != 6 && length != 8)
            return false;

        int digits[8];
        for (size_t i = 0; i < length; ++i)
        {
            const char c = text[i];
            if (c >= '0' && c <= '9')
                digits[i] = c - '0';
            else if (c >= 'a' && c <= 'f')
                digits[i] = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                digits[i] = c - 'A' + 10;
            else
                return false;
        }

        out[3] = 255;
        if (length <= 4)
        {
            for (size_t i = 0; i < length; ++i)
                out[i] = static_cast<Uint8>(digits[i] * 17);
        }
        else
        {
            for (size_t i = 0; i < length / 2; ++i)
                out[i] = static_cast<Uint8>(digits[i * 2] * 16 + digits[i * 2 + 1]);
        }
        return true;
    }

    bool ReadColorValue(lua_State *L, int index, SDL_Color &out)
    {
        index = lua_absindex(L, index);
        const int type = lua_type(L, index);
        if (type == LUA_TSTRING)
        {
            const char *text = lua_tostring(L, index);
            if (text[0] != '#' && LookupColorName(text, out))
                return true;
            Uint8 rgba[4];
            if (!ParseHexColor(text, rgba))
                return false;
            out = SDL_Color{rgba[0], rgba[1], rgba[2], rgba[3]};
            return true;
        }
        if (type != LUA_TTABLE)
            return false;

        double c[4] = {255.0, 255.0, 255.0, 255.0};
        for (int i = 0; i < 4; ++i)
        {
            lua_rawgeti(L, index, i + 1);
            if (lua_isnumber(L, -1))
                c[i] = lua_tonumber(L, -1);
            lua_pop(L, 1);
        }
        auto byte = [](double v)
        { return static_cast<Uint8>(std::clamp(std::round(v), 0.0, 255.0)); };
        out = SDL_Color{byte(c[0]), byte(c[1]), byte(c[2]), byte(c[3])};
        return true;
    }

    namespace
    {
        constexpr const char *ColorHint = "a color is {r, g, b, a} (0-255), \"#rrggbb\" or a name such as \"gold\"";

        SDL_Color CheckColor(lua_State *L, int index, const char *function)
        {
            SDL_Color color{255, 255, 255, 255};
            if (!ReadColorValue(L, index, color))
                luaL_error(L, "%s: bad color in argument %d (%s; names: %s)", function, index, ColorHint, ColorNameList());
            return color;
        }

        Uint8 FractionToByte(double a)
        {
            return static_cast<Uint8>(std::clamp(std::round(a * 255.0), 0.0, 255.0));
        }

        void PushColor(lua_State *L, const SDL_Color &color)
        {
            lua_createtable(L, 4, 0);
            lua_pushinteger(L, color.r);
            lua_rawseti(L, -2, 1);
            lua_pushinteger(L, color.g);
            lua_rawseti(L, -2, 2);
            lua_pushinteger(L, color.b);
            lua_rawseti(L, -2, 3);
            lua_pushinteger(L, color.a);
            lua_rawseti(L, -2, 4);
        }

        int ColorNew(lua_State *L)
        {
            const SDL_Color color{ToByte(L, 1), ToByte(L, 2), ToByte(L, 3), ToByteOpt(L, 4, 255)};
            PushColor(L, color);
            return 1;
        }

        int ColorHex(lua_State *L)
        {
            const char *text = luaL_checkstring(L, 1);
            const bool hasAlpha = !lua_isnoneornil(L, 2);
            const double alpha = hasAlpha ? luaL_checknumber(L, 2) : 1.0;
            Uint8 rgba[4];
            if (!ParseHexColor(text, rgba))
                return luaL_error(L, "Color.Hex: bad color '%s' (use #rgb, #rrggbb or #rrggbbaa)", text);
            if (hasAlpha)
                rgba[3] = FractionToByte(alpha);
            PushColor(L, SDL_Color{rgba[0], rgba[1], rgba[2], rgba[3]});
            return 1;
        }

        int ColorMix(lua_State *L)
        {
            const SDL_Color a = CheckColor(L, 1, "Color.Mix");
            const SDL_Color b = CheckColor(L, 2, "Color.Mix");
            const double t = std::clamp(luaL_checknumber(L, 3), 0.0, 1.0);
            auto mix = [t](Uint8 x, Uint8 y)
            { return static_cast<Uint8>(std::clamp(std::round(x + (y - x) * t), 0.0, 255.0)); };
            PushColor(L, SDL_Color{mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a)});
            return 1;
        }

        int ColorWithAlpha(lua_State *L)
        {
            SDL_Color color = CheckColor(L, 1, "Color.WithAlpha");
            color.a = FractionToByte(luaL_checknumber(L, 2));
            PushColor(L, color);
            return 1;
        }

        int ColorUnpack(lua_State *L)
        {
            SDL_Color color = CheckColor(L, 1, "Color.Unpack");
            if (!lua_isnoneornil(L, 2))
                color.a = FractionToByte(luaL_checknumber(L, 2));
            return PushRgba(L, color);
        }

        int ColorDefine(lua_State *L)
        {
            const char *name = luaL_checkstring(L, 1);
            SDL_Color color = CheckColor(L, 2, "Color.Define");
            if (name[0] == '\0' || name[0] == '#')
                return luaL_error(L, "Color.Define: a color name cannot be empty or start with '#'");
            Palette()[name] = color;
            PushColor(L, color);
            return 1;
        }

        int ColorNames(lua_State *L)
        {
            lua_newtable(L);
            lua_Integer n = 0;
            for (const auto &[name, color] : Palette())
            {
                lua_pushlstring(L, name.data(), name.size());
                lua_rawseti(L, -2, ++n);
            }
            return 1;
        }

        int ColorGet(lua_State *L)
        {
            PushColor(L, CheckColor(L, 1, "Color.Get"));
            return 1;
        }

        const luaL_Reg ColorFunctions[] = {
            {"Define", ColorDefine},
            {"Names", ColorNames},
            {"Get", ColorGet},
            {"New", ColorNew},
            {"Hex", ColorHex},
            {"Mix", ColorMix},
            {"WithAlpha", ColorWithAlpha},
            {"Unpack", ColorUnpack},
            {nullptr, nullptr}};
    }

    void RegisterColor(lua_State *L)
    {
        ResetPalette();
        luaL_newlib(L, ColorFunctions);
        lua_setglobal(L, "Color");
    }
}
