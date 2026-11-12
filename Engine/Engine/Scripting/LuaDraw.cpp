#include "LuaModules.h"

#include "LuaHelpers.h"

#include <Engine/Rendering/Font.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Vector/VectorShape.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        bool s_Active = false;
        float s_Z = 0.0f;

        void Require(lua_State *L)
        {
            if (!s_Active)
                luaL_error(L, "Draw.* can only be used inside OnRender");
        }

        float Num(lua_State *L, int index)
        {
            return static_cast<float>(luaL_checknumber(L, index));
        }

        SDL_Color ColorArgs(lua_State *L, int first)
        {
            return SDL_Color{ToByte(L, first), ToByte(L, first + 1), ToByte(L, first + 2), ToByteOpt(L, first + 3, 255)};
        }

        glm::vec4 ToVec(const SDL_Color &c)
        {
            return glm::vec4(c.r, c.g, c.b, c.a) / 255.0f;
        }

        bool ReadPoints(lua_State *L, int index, std::vector<glm::vec2> &out)
        {
            const lua_Integer count = static_cast<lua_Integer>(lua_rawlen(L, index));
            if (count == 0)
                return true;
            lua_rawgeti(L, index, 1);
            const bool nested = lua_type(L, -1) == LUA_TTABLE;
            lua_pop(L, 1);
            if (nested)
            {
                for (lua_Integer i = 1; i <= count; ++i)
                {
                    lua_rawgeti(L, index, i);
                    lua_rawgeti(L, -1, 1);
                    lua_rawgeti(L, -2, 2);
                    out.push_back({static_cast<float>(lua_tonumber(L, -2)), static_cast<float>(lua_tonumber(L, -1))});
                    lua_pop(L, 3);
                }
                return true;
            }
            if (count % 2 != 0)
                return false;
            for (lua_Integer i = 1; i + 1 <= count; i += 2)
            {
                lua_rawgeti(L, index, i);
                lua_rawgeti(L, index, i + 1);
                out.push_back({static_cast<float>(lua_tonumber(L, -2)), static_cast<float>(lua_tonumber(L, -1))});
                lua_pop(L, 2);
            }
            return true;
        }

        bool ReadColorTable(lua_State *L, int index, glm::vec4 &out)
        {
            const int type = lua_type(L, index);
            if (type != LUA_TTABLE && type != LUA_TSTRING)
                return false;
            SDL_Color color{255, 255, 255, 255};
            if (!ReadColorValue(L, index, color))
                return false;
            out = glm::vec4(color.r, color.g, color.b, color.a) / 255.0f;
            return true;
        }

        SDL_Color ColorAt(lua_State *L, int first, int &next)
        {
            const int type = lua_type(L, first);
            if (type == LUA_TTABLE || (type == LUA_TSTRING && !lua_isnumber(L, first)))
            {
                SDL_Color color{255, 255, 255, 255};
                if (!ReadColorValue(L, first, color))
                    luaL_error(L, "Draw: bad color '%s' (use r, g, b, a, {r, g, b, a}, \"#rrggbb\" or a name: %s)", type == LUA_TSTRING ? lua_tostring(L, first) : "table", ColorNameList());
                next = first + 1;
                return color;
            }
            next = first + 4;
            return ColorArgs(L, first);
        }

        StrokeCap CapFromName(const char *name)
        {
            if (std::strcmp(name, "butt") == 0)
                return StrokeCap::Butt;
            if (std::strcmp(name, "square") == 0)
                return StrokeCap::Square;
            return StrokeCap::Round;
        }

        StrokeJoin JoinFromName(const char *name)
        {
            if (std::strcmp(name, "miter") == 0)
                return StrokeJoin::Miter;
            if (std::strcmp(name, "bevel") == 0)
                return StrokeJoin::Bevel;
            return StrokeJoin::Round;
        }

        int DrawLine(lua_State *L)
        {
            Require(L);
            const float x1 = Num(L, 1), y1 = Num(L, 2), x2 = Num(L, 3), y2 = Num(L, 4), w = Num(L, 5);
            int next = 0;
            Renderer::DrawCapsuleLine(x1, y1, x2, y2, w, ColorAt(L, 6, next), 1.0f, s_Z);
            return 0;
        }

        int DrawCircle(lua_State *L)
        {
            Require(L);
            const float x = Num(L, 1), y = Num(L, 2), r = Num(L, 3);
            int next = 0;
            Renderer::DrawCircle(x, y, r, ColorAt(L, 4, next), 1.0f, s_Z);
            return 0;
        }

        int DrawRing(lua_State *L)
        {
            Require(L);
            const float x = Num(L, 1), y = Num(L, 2), r = Num(L, 3), t = Num(L, 4);
            int next = 0;
            Renderer::DrawRing(x, y, r, t, ColorAt(L, 5, next), 1.0f, s_Z);
            return 0;
        }

        int DrawRect(lua_State *L)
        {
            Require(L);
            const float x = Num(L, 1), y = Num(L, 2), w = Num(L, 3), h = Num(L, 4);
            int next = 0;
            const SDL_Color color = ColorAt(L, 5, next);
            const float rotation = static_cast<float>(luaL_optnumber(L, next, 0.0));
            Renderer::DrawQuad(x, y, w, h, rotation, color, true, s_Z);
            return 0;
        }

        int DrawRoundRect(lua_State *L)
        {
            Require(L);
            const float x = Num(L, 1), y = Num(L, 2), w = Num(L, 3), h = Num(L, 4), r = Num(L, 5);
            int next = 0;
            Renderer::DrawRoundedRect(x, y, w * 0.5f, h * 0.5f, r, ColorAt(L, 6, next), 1.0f, s_Z);
            return 0;
        }

        int DrawPolyline(lua_State *L)
        {
            Require(L);
            luaL_checktype(L, 1, LUA_TTABLE);
            const float width = Num(L, 2);
            int next = 0;
            const SDL_Color color = ColorAt(L, 3, next);
            const bool closed = lua_toboolean(L, next) != 0;
            bool ok = true;
            {
                std::vector<glm::vec2> points;
                ok = ReadPoints(L, 1, points);
                if (ok && points.size() >= 2)
                {
                    StrokeStyle style;
                    style.Width = width;
                    VectorShapes::DrawPolyline(points, closed, style, ToVec(color), s_Z);
                }
            }
            if (!ok)
                return luaL_error(L, "Draw.Polyline: expected {x1, y1, x2, y2, ...} or {{x, y}, ...}");
            return 0;
        }

        int DrawPolygon(lua_State *L)
        {
            Require(L);
            luaL_checktype(L, 1, LUA_TTABLE);
            int next = 0;
            const SDL_Color color = ColorAt(L, 2, next);
            bool ok = true;
            {
                std::vector<glm::vec2> points;
                ok = ReadPoints(L, 1, points);
                if (ok && points.size() >= 3)
                    VectorShapes::DrawPolygon(points, ToVec(color), s_Z);
            }
            if (!ok)
                return luaL_error(L, "Draw.Polygon: expected {x1, y1, x2, y2, ...} or {{x, y}, ...}");
            return 0;
        }

        int DrawBezier(lua_State *L)
        {
            Require(L);
            float v[8];
            for (int i = 0; i < 8; ++i)
                v[i] = Num(L, i + 1);
            const float width = Num(L, 9);
            int next = 0;
            const SDL_Color color = ColorAt(L, 10, next);
            {
                VectorPath path;
                path.MoveTo(v[0], v[1]).CubicTo(v[2], v[3], v[4], v[5], v[6], v[7]);
                StrokeStyle style;
                style.Width = width;
                VectorShapes::DrawPath(path, glm::vec2(0.0f), 0.0f, glm::vec2(1.0f), s_Z, false, glm::vec4(0.0f), true, ToVec(color), style);
            }
            return 0;
        }

        int DrawPathFn(lua_State *L)
        {
            Require(L);
            VectorPath *path = TestPath(L, 1);
            if (!path)
                return luaL_error(L, "Draw.Path: argument 1 must be a Path");
            const float x = static_cast<float>(luaL_optnumber(L, 2, 0.0));
            const float y = static_cast<float>(luaL_optnumber(L, 3, 0.0));
            const int styleIndex = 4;
            {
                bool fill = true;
                glm::vec4 fillColor(1.0f);
                bool stroke = false;
                glm::vec4 strokeColor(0.0f, 0.0f, 0.0f, 1.0f);
                StrokeStyle style;
                float rotation = 0.0f;
                glm::vec2 scale(1.0f);
                float trim[3] = {0.0f, 1.0f, 0.0f};
                if (lua_type(L, styleIndex) == LUA_TTABLE)
                {
                    lua_getfield(L, styleIndex, "fill");
                    if (lua_isboolean(L, -1))
                        fill = lua_toboolean(L, -1) != 0;
                    else
                        ReadColorTable(L, lua_gettop(L), fillColor);
                    lua_pop(L, 1);
                    lua_getfield(L, styleIndex, "stroke");
                    stroke = ReadColorTable(L, lua_gettop(L), strokeColor);
                    lua_pop(L, 1);
                    lua_getfield(L, styleIndex, "width");
                    if (lua_isnumber(L, -1))
                        style.Width = static_cast<float>(lua_tonumber(L, -1));
                    lua_pop(L, 1);
                    lua_getfield(L, styleIndex, "cap");
                    if (lua_type(L, -1) == LUA_TSTRING)
                        style.Cap = CapFromName(lua_tostring(L, -1));
                    lua_pop(L, 1);
                    lua_getfield(L, styleIndex, "join");
                    if (lua_type(L, -1) == LUA_TSTRING)
                        style.Join = JoinFromName(lua_tostring(L, -1));
                    lua_pop(L, 1);
                    lua_getfield(L, styleIndex, "rotation");
                    if (lua_isnumber(L, -1))
                        rotation = static_cast<float>(lua_tonumber(L, -1));
                    lua_pop(L, 1);
                    lua_getfield(L, styleIndex, "scale");
                    if (lua_isnumber(L, -1))
                        scale = glm::vec2(static_cast<float>(lua_tonumber(L, -1)));
                    else if (lua_type(L, -1) == LUA_TTABLE)
                    {
                        lua_rawgeti(L, -1, 1);
                        lua_rawgeti(L, -2, 2);
                        scale = {static_cast<float>(lua_tonumber(L, -2)), static_cast<float>(lua_tonumber(L, -1))};
                        lua_pop(L, 2);
                    }
                    lua_pop(L, 1);
                    lua_getfield(L, styleIndex, "trim");
                    if (lua_type(L, -1) == LUA_TTABLE)
                        for (int i = 0; i < 3; ++i)
                        {
                            lua_rawgeti(L, -1, i + 1);
                            if (lua_isnumber(L, -1))
                                trim[i] = static_cast<float>(lua_tonumber(L, -1));
                            lua_pop(L, 1);
                        }
                    lua_pop(L, 1);
                }
                VectorShapes::DrawPath(*path, {x, y}, rotation, scale, s_Z, fill, fillColor, stroke, strokeColor, style, trim[0], trim[1], trim[2]);
            }
            return 0;
        }

        int DrawTextAt(lua_State *L, bool middleByDefault)
        {
            Require(L);
            size_t length = 0;
            const char *text = luaL_checklstring(L, 1, &length);
            const float x = Num(L, 2), y = Num(L, 3);
            float size = 32.0f;
            SDL_Color color{255, 255, 255, 255};
            const char *align = "left";
            bool middle = middleByDefault;
            if (lua_type(L, 4) == LUA_TTABLE)
            {
                lua_getfield(L, 4, "size");
                if (lua_isnumber(L, -1))
                    size = static_cast<float>(lua_tonumber(L, -1));
                lua_pop(L, 1);
                lua_getfield(L, 4, "color");
                const bool hasColor = !lua_isnil(L, -1);
                const bool colorOk = !hasColor || ReadColorValue(L, -1, color);
                lua_pop(L, 1);
                if (!colorOk)
                    return luaL_error(L, "Draw.Text: bad color in the options table (names: %s)", ColorNameList());
                lua_getfield(L, 4, "align");
                if (lua_isstring(L, -1))
                    align = lua_tostring(L, -1);
                lua_pop(L, 1);
                lua_getfield(L, 4, "middle");
                if (lua_isboolean(L, -1))
                    middle = lua_toboolean(L, -1) != 0;
                lua_pop(L, 1);
            }
            else
            {
                size = Num(L, 4);
                int next = 0;
                color = ColorAt(L, 5, next);
                align = luaL_optstring(L, next, "left");
            }
            TextAlign a = TextAlign::Left;
            if (std::strcmp(align, "center") == 0)
                a = TextAlign::Center;
            else if (std::strcmp(align, "right") == 0)
                a = TextAlign::Right;
            Renderer::DrawText(std::string(text, length), x, y + (middle ? size * 0.52f : 0.0f), size, color, a, 0.0f, s_Z);
            return 0;
        }

        int DrawText(lua_State *L)
        {
            return DrawTextAt(L, false);
        }

        int DrawLabel(lua_State *L)
        {
            return DrawTextAt(L, true);
        }

        int DrawDashed(lua_State *L)
        {
            Require(L);
            const float x1 = Num(L, 1), y1 = Num(L, 2), x2 = Num(L, 3), y2 = Num(L, 4), w = Num(L, 5);
            float dash = Num(L, 6), gap = Num(L, 7);
            int next = 0;
            const SDL_Color color = ColorAt(L, 8, next);
            const float progress = std::clamp(static_cast<float>(luaL_optnumber(L, next, 1.0)), 0.0f, 1.0f);
            const float dx = x2 - x1, dy = y2 - y1;
            const float length = std::sqrt(dx * dx + dy * dy);
            if (length < 0.5f || dash <= 0.0f)
                return 0;
            gap = std::max(gap, 0.0f);
            const float reach = length * progress;
            float period = dash + gap;
            if (reach / period > 90.0f)
            {
                const float k = reach / period / 90.0f;
                dash *= k;
                gap *= k;
                period *= k;
            }
            const float ux = dx / length, uy = dy / length;
            for (float d = 0.0f; d < reach; d += period)
            {
                const float e = std::min(d + dash, reach);
                Renderer::DrawCapsuleLine(x1 + ux * d, y1 + uy * d, x1 + ux * e, y1 + uy * e, w, color, 1.0f, s_Z);
            }
            return 0;
        }

        int DrawArrow(lua_State *L)
        {
            Require(L);
            const float x1 = Num(L, 1), y1 = Num(L, 2), x2 = Num(L, 3), y2 = Num(L, 4), w = Num(L, 5);
            int next = 0;
            const SDL_Color color = ColorAt(L, 6, next);
            const float head = static_cast<float>(luaL_optnumber(L, next, w * 4.2));
            Renderer::DrawCapsuleLine(x1, y1, x2, y2, w, color, 1.0f, s_Z);
            const float angle = std::atan2(y2 - y1, x2 - x1);
            for (float side : {1.0f, -1.0f})
            {
                const float a = angle + 3.14159265f + side * 0.5f;
                Renderer::DrawCapsuleLine(x2, y2, x2 + std::cos(a) * head, y2 + std::sin(a) * head, w, color, 1.0f, s_Z);
            }
            return 0;
        }

        int DrawMeasureText(lua_State *L)
        {
            size_t length = 0;
            const char *text = luaL_checklstring(L, 1, &length);
            const float size = Num(L, 2);
            const char *fontPath = luaL_optstring(L, 3, "");
            glm::vec2 measured(0.0f);
            {
                std::shared_ptr<Font> font = fontPath[0] ? Font::Load(fontPath) : Font::GetDefault();
                if (font && size > 0.0f)
                    measured = font->MeasureText(std::string(text, length), size);
            }
            lua_pushnumber(L, measured.x);
            lua_pushnumber(L, measured.y);
            return 2;
        }

        int DrawSetZ(lua_State *L)
        {
            s_Z = Num(L, 1);
            return 0;
        }

        int DrawGetZ(lua_State *L)
        {
            lua_pushnumber(L, s_Z);
            return 1;
        }

        const luaL_Reg DrawFunctions[] = {
            {"Line", DrawLine},
            {"Circle", DrawCircle},
            {"Ring", DrawRing},
            {"Rect", DrawRect},
            {"RoundRect", DrawRoundRect},
            {"Polyline", DrawPolyline},
            {"Polygon", DrawPolygon},
            {"Bezier", DrawBezier},
            {"Path", DrawPathFn},
            {"Text", DrawText},
            {"Label", DrawLabel},
            {"Dashed", DrawDashed},
            {"Arrow", DrawArrow},
            {"MeasureText", DrawMeasureText},
            {"SetZ", DrawSetZ},
            {"GetZ", DrawGetZ},
            {nullptr, nullptr}};
    }

    void BeginDraw(float zIndex)
    {
        s_Active = true;
        s_Z = zIndex;
    }

    void EndDraw()
    {
        s_Active = false;
    }

    void RegisterDraw(lua_State *L)
    {
        luaL_newlib(L, DrawFunctions);
        lua_setglobal(L, "Draw");
    }
}
