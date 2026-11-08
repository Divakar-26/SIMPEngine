#include "LuaModules.h"

#include "LuaHelpers.h"

#include <Engine/Rendering/Renderer.h>
#include <Engine/Vector/VectorShape.h>

#include <cstring>
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
            if (lua_type(L, index) != LUA_TTABLE)
                return false;
            float c[4] = {255.0f, 255.0f, 255.0f, 255.0f};
            for (int i = 0; i < 4; ++i)
            {
                lua_rawgeti(L, index, i + 1);
                if (lua_isnumber(L, -1))
                    c[i] = static_cast<float>(lua_tonumber(L, -1));
                lua_pop(L, 1);
            }
            out = glm::vec4(c[0], c[1], c[2], c[3]) / 255.0f;
            return true;
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
            Renderer::DrawCapsuleLine(x1, y1, x2, y2, w, ColorArgs(L, 6), 1.0f, s_Z);
            return 0;
        }

        int DrawCircle(lua_State *L)
        {
            Require(L);
            const float x = Num(L, 1), y = Num(L, 2), r = Num(L, 3);
            Renderer::DrawCircle(x, y, r, ColorArgs(L, 4), 1.0f, s_Z);
            return 0;
        }

        int DrawRing(lua_State *L)
        {
            Require(L);
            const float x = Num(L, 1), y = Num(L, 2), r = Num(L, 3), t = Num(L, 4);
            Renderer::DrawRing(x, y, r, t, ColorArgs(L, 5), 1.0f, s_Z);
            return 0;
        }

        int DrawRect(lua_State *L)
        {
            Require(L);
            const float x = Num(L, 1), y = Num(L, 2), w = Num(L, 3), h = Num(L, 4);
            const SDL_Color color = ColorArgs(L, 5);
            const float rotation = static_cast<float>(luaL_optnumber(L, 9, 0.0));
            Renderer::DrawQuad(x, y, w, h, rotation, color, true, s_Z);
            return 0;
        }

        int DrawRoundRect(lua_State *L)
        {
            Require(L);
            const float x = Num(L, 1), y = Num(L, 2), w = Num(L, 3), h = Num(L, 4), r = Num(L, 5);
            Renderer::DrawRoundedRect(x, y, w * 0.5f, h * 0.5f, r, ColorArgs(L, 6), 1.0f, s_Z);
            return 0;
        }

        int DrawPolyline(lua_State *L)
        {
            Require(L);
            luaL_checktype(L, 1, LUA_TTABLE);
            const float width = Num(L, 2);
            const SDL_Color color = ColorArgs(L, 3);
            const bool closed = lua_toboolean(L, 7) != 0;
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
            const SDL_Color color = ColorArgs(L, 2);
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
            const SDL_Color color = ColorArgs(L, 10);
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

        int DrawText(lua_State *L)
        {
            Require(L);
            size_t length = 0;
            const char *text = luaL_checklstring(L, 1, &length);
            const float x = Num(L, 2), y = Num(L, 3), size = Num(L, 4);
            const SDL_Color color = ColorArgs(L, 5);
            const char *align = luaL_optstring(L, 9, "left");
            TextAlign a = TextAlign::Left;
            if (std::strcmp(align, "center") == 0)
                a = TextAlign::Center;
            else if (std::strcmp(align, "right") == 0)
                a = TextAlign::Right;
            Renderer::DrawText(std::string(text, length), x, y, size, color, a, 0.0f, s_Z);
            return 0;
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
