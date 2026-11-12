#include "LuaModules.h"

#include "LuaHelpers.h"

#include <Engine/Rendering/Font.h>
#include <Engine/Vector/TextOutline.h>
#include <Engine/Vector/VectorPath.h>

#include <cmath>
#include <cstring>
#include <new>
#include <string>
#include <vector>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *PathMeta = "SIMP.VectorPath";

        VectorPath *CheckPath(lua_State *L, int index)
        {
            return static_cast<VectorPath *>(luaL_checkudata(L, index, PathMeta));
        }

        float Num(lua_State *L, int index)
        {
            return static_cast<float>(luaL_checknumber(L, index));
        }

        int Self(lua_State *L)
        {
            lua_pushvalue(L, 1);
            return 1;
        }

        int PathGc(lua_State *L)
        {
            CheckPath(L, 1)->~VectorPath();
            return 0;
        }

        int PathMoveTo(lua_State *L)
        {
            VectorPath *p = CheckPath(L, 1);
            const float x = Num(L, 2), y = Num(L, 3);
            p->MoveTo(x, y);
            return Self(L);
        }

        int PathLineTo(lua_State *L)
        {
            VectorPath *p = CheckPath(L, 1);
            const float x = Num(L, 2), y = Num(L, 3);
            p->LineTo(x, y);
            return Self(L);
        }

        int PathQuadTo(lua_State *L)
        {
            VectorPath *p = CheckPath(L, 1);
            const float cx = Num(L, 2), cy = Num(L, 3), x = Num(L, 4), y = Num(L, 5);
            p->QuadTo(cx, cy, x, y);
            return Self(L);
        }

        int PathCubicTo(lua_State *L)
        {
            VectorPath *p = CheckPath(L, 1);
            const float a = Num(L, 2), b = Num(L, 3), c = Num(L, 4), d = Num(L, 5), x = Num(L, 6), y = Num(L, 7);
            p->CubicTo(a, b, c, d, x, y);
            return Self(L);
        }

        int PathArcTo(lua_State *L)
        {
            VectorPath *p = CheckPath(L, 1);
            const float cx = Num(L, 2), cy = Num(L, 3), r = Num(L, 4), a0 = Num(L, 5), a1 = Num(L, 6);
            p->ArcTo(cx, cy, r, a0, a1);
            return Self(L);
        }

        int PathClose(lua_State *L)
        {
            CheckPath(L, 1)->Close();
            return Self(L);
        }

        int PathAppend(lua_State *L)
        {
            VectorPath *p = CheckPath(L, 1);
            VectorPath *other = CheckPath(L, 2);
            p->Append(*other);
            return Self(L);
        }

        int PathTranslate(lua_State *L)
        {
            VectorPath *p = CheckPath(L, 1);
            const float x = Num(L, 2), y = Num(L, 3);
            p->Translate(x, y);
            return Self(L);
        }

        int PathScale(lua_State *L)
        {
            VectorPath *p = CheckPath(L, 1);
            const float x = Num(L, 2);
            const float y = static_cast<float>(luaL_optnumber(L, 3, x));
            p->Scale(x, y);
            return Self(L);
        }

        int PathRotate(lua_State *L)
        {
            VectorPath *p = CheckPath(L, 1);
            p->Rotate(Num(L, 2));
            return Self(L);
        }

        int PathCopy(lua_State *L)
        {
            PushPath(L, *CheckPath(L, 1));
            return 1;
        }

        int PathGetBounds(lua_State *L)
        {
            glm::vec2 min, max;
            if (!CheckPath(L, 1)->GetBounds(min, max))
            {
                lua_pushnil(L);
                return 1;
            }
            lua_pushnumber(L, min.x);
            lua_pushnumber(L, min.y);
            lua_pushnumber(L, max.x);
            lua_pushnumber(L, max.y);
            return 4;
        }

        int PathGetLength(lua_State *L)
        {
            VectorPath *p = CheckPath(L, 1);
            float total = 0.0f;
            {
                std::vector<FlatContour> flat;
                p->Flatten(0.25f, flat);
                for (const FlatContour &c : flat)
                    total += PolylineLength(c.Points, c.Closed);
            }
            lua_pushnumber(L, total);
            return 1;
        }

        int PathGetPointAt(lua_State *L)
        {
            VectorPath *p = CheckPath(L, 1);
            const float t = Num(L, 2);
            bool ok = false;
            glm::vec2 point(0.0f), tangent(1.0f, 0.0f);
            {
                std::vector<FlatContour> flat;
                p->Flatten(0.25f, flat);
                if (!flat.empty())
                {
                    const float length = PolylineLength(flat[0].Points, flat[0].Closed);
                    ok = PointAtDistance(flat[0].Points, flat[0].Closed, t * length, point, tangent);
                }
            }
            if (!ok)
                return 0;
            lua_pushnumber(L, point.x);
            lua_pushnumber(L, point.y);
            lua_pushnumber(L, glm::degrees(std::atan2(tangent.y, tangent.x)));
            return 3;
        }

        int PathIsEmpty(lua_State *L)
        {
            lua_pushboolean(L, CheckPath(L, 1)->Empty());
            return 1;
        }

        int PathContourCount(lua_State *L)
        {
            lua_pushinteger(L, static_cast<lua_Integer>(CheckPath(L, 1)->Contours.size()));
            return 1;
        }

        int PathToString(lua_State *L)
        {
            lua_pushfstring(L, "Path(%d contours)", static_cast<int>(CheckPath(L, 1)->Contours.size()));
            return 1;
        }

        int NewPath(lua_State *L)
        {
            PushPath(L, VectorPath());
            return 1;
        }

        int MakeRect(lua_State *L)
        {
            const float w = Num(L, 1), h = Num(L, 2);
            const float r = static_cast<float>(luaL_optnumber(L, 3, 0.0));
            PushPath(L, VectorPath::Rect(w, h, r));
            return 1;
        }

        int MakeCircle(lua_State *L)
        {
            PushPath(L, VectorPath::Circle(Num(L, 1)));
            return 1;
        }

        int MakeEllipse(lua_State *L)
        {
            const float rx = Num(L, 1), ry = Num(L, 2);
            PushPath(L, VectorPath::Ellipse(rx, ry));
            return 1;
        }

        int MakePolygon(lua_State *L)
        {
            const int sides = static_cast<int>(luaL_checkinteger(L, 1));
            PushPath(L, VectorPath::Polygon(sides, Num(L, 2)));
            return 1;
        }

        int MakeStar(lua_State *L)
        {
            const int points = static_cast<int>(luaL_checkinteger(L, 1));
            const float outer = Num(L, 2);
            const float inner = static_cast<float>(luaL_optnumber(L, 3, outer * 0.45f));
            PushPath(L, VectorPath::Star(points, outer, inner));
            return 1;
        }

        int MakeHeart(lua_State *L)
        {
            PushPath(L, VectorPath::Heart(Num(L, 1)));
            return 1;
        }

        int MakeLine(lua_State *L)
        {
            const float a = Num(L, 1), b = Num(L, 2), c = Num(L, 3), d = Num(L, 4);
            PushPath(L, VectorPath::Line(a, b, c, d));
            return 1;
        }

        int MakeArc(lua_State *L)
        {
            const float r = Num(L, 1), a0 = Num(L, 2), a1 = Num(L, 3);
            PushPath(L, VectorPath::Arc(r, a0, a1));
            return 1;
        }

        int MakePoints(lua_State *L)
        {
            luaL_checktype(L, 1, LUA_TTABLE);
            const bool closed = lua_isnoneornil(L, 2) ? true : lua_toboolean(L, 2) != 0;
            const lua_Integer count = luaL_len(L, 1);
            bool odd = false;
            {
                std::vector<glm::vec2> points;
                lua_rawgeti(L, 1, 1);
                const bool nested = lua_type(L, -1) == LUA_TTABLE;
                lua_pop(L, 1);
                if (nested)
                {
                    for (lua_Integer i = 1; i <= count; ++i)
                    {
                        lua_rawgeti(L, 1, i);
                        lua_rawgeti(L, -1, 1);
                        lua_rawgeti(L, -2, 2);
                        points.push_back({static_cast<float>(lua_tonumber(L, -2)), static_cast<float>(lua_tonumber(L, -1))});
                        lua_pop(L, 3);
                    }
                }
                else if (count % 2 != 0)
                    odd = true;
                else
                {
                    for (lua_Integer i = 1; i + 1 <= count; i += 2)
                    {
                        lua_rawgeti(L, 1, i);
                        lua_rawgeti(L, 1, i + 1);
                        points.push_back({static_cast<float>(lua_tonumber(L, -2)), static_cast<float>(lua_tonumber(L, -1))});
                        lua_pop(L, 2);
                    }
                }
                if (!odd)
                    PushPath(L, VectorPath::Points(points, closed));
            }
            if (odd)
                return luaL_error(L, "Path.Points: expected {x1, y1, x2, y2, ...} or {{x, y}, ...}");
            return 1;
        }

        int MakeSvg(lua_State *L)
        {
            size_t length = 0;
            const char *source = luaL_checklstring(L, 1, &length);
            const float size = static_cast<float>(luaL_optnumber(L, 2, 0.0));
            bool ok = false;
            {
                const std::string text(source, length);
                VectorPath path;
                const bool isFile = text.rfind("assets://", 0) == 0 || (text.size() > 4 && text.compare(text.size() - 4, 4, ".svg") == 0);
                if (isFile)
                    ok = path.LoadSvgFile(text, size);
                else if (text.find("<svg") != std::string::npos)
                    ok = path.LoadSvgText(text, size);
                else
                    ok = path.LoadSvgPathData(text, size);
                if (ok)
                    PushPath(L, path);
            }
            if (!ok)
            {
                lua_pushnil(L);
                lua_pushstring(L, "could not read the SVG");
                return 2;
            }
            return 1;
        }

        int MakeText(lua_State *L)
        {
            size_t length = 0;
            const char *source = luaL_checklstring(L, 1, &length);
            const float size = Num(L, 2);
            const char *alignName = luaL_optstring(L, 3, "left");
            size_t fontLength = 0;
            const char *fontPath = luaL_optlstring(L, 4, "", &fontLength);
            const TextAlign align = std::strcmp(alignName, "center") == 0 ? TextAlign::Center : std::strcmp(alignName, "right") == 0 ? TextAlign::Right : TextAlign::Left;
            bool ok = false;
            bool noFont = false;
            {
                std::shared_ptr<Font> font = fontLength > 0 ? Font::Load(std::string(fontPath, fontLength)) : Font::GetDefault();
                noFont = !font;
                TextOutline outline;
                if (font)
                    ok = BuildTextOutline(*font, std::string(source, length), size, align, outline);
                if (ok)
                    PushPath(L, outline.Path);
            }
            if (!ok)
            {
                lua_pushnil(L);
                lua_pushstring(L, noFont ? "font not found" : "no visible glyphs");
                return 2;
            }
            return 1;
        }

        int MakeTextGlyphs(lua_State *L)
        {
            size_t length = 0;
            const char *source = luaL_checklstring(L, 1, &length);
            const float size = Num(L, 2);
            const char *alignName = luaL_optstring(L, 3, "left");
            size_t fontLength = 0;
            const char *fontPath = luaL_optlstring(L, 4, "", &fontLength);
            const TextAlign align = std::strcmp(alignName, "center") == 0 ? TextAlign::Center : std::strcmp(alignName, "right") == 0 ? TextAlign::Right : TextAlign::Left;
            bool ok = false;
            bool noFont = false;
            {
                std::shared_ptr<Font> font = fontLength > 0 ? Font::Load(std::string(fontPath, fontLength)) : Font::GetDefault();
                noFont = !font;
                TextOutline outline;
                if (font)
                    ok = BuildTextOutline(*font, std::string(source, length), size, align, outline);
                if (ok)
                {
                    lua_createtable(L, static_cast<int>(outline.Glyphs.size()), 0);
                    for (size_t i = 0; i < outline.Glyphs.size(); ++i)
                    {
                        const TextOutlineGlyph &g = outline.Glyphs[i];
                        lua_createtable(L, 0, 8);
                        PushPath(L, outline.GlyphPath(i));
                        lua_setfield(L, -2, "path");
                        auto number = [&](const char *key, lua_Number value)
                        {
                            lua_pushnumber(L, value);
                            lua_setfield(L, -2, key);
                        };
                        number("index", g.Index + 1);
                        number("codepoint", g.Codepoint);
                        number("line", g.Line + 1);
                        number("minX", g.Min.x);
                        number("minY", g.Min.y);
                        number("maxX", g.Max.x);
                        number("maxY", g.Max.y);
                        lua_rawseti(L, -2, static_cast<lua_Integer>(i + 1));
                    }
                }
            }
            if (!ok)
            {
                lua_pushnil(L);
                lua_pushstring(L, noFont ? "font not found" : "no visible glyphs");
                return 2;
            }
            return 1;
        }

        const luaL_Reg PathMethods[] = {
            {"MoveTo", PathMoveTo},
            {"LineTo", PathLineTo},
            {"QuadTo", PathQuadTo},
            {"CubicTo", PathCubicTo},
            {"ArcTo", PathArcTo},
            {"Close", PathClose},
            {"Append", PathAppend},
            {"Translate", PathTranslate},
            {"Scale", PathScale},
            {"Rotate", PathRotate},
            {"Copy", PathCopy},
            {"GetBounds", PathGetBounds},
            {"GetLength", PathGetLength},
            {"GetPointAt", PathGetPointAt},
            {"IsEmpty", PathIsEmpty},
            {"ContourCount", PathContourCount},
            {nullptr, nullptr}};

        const luaL_Reg PathFunctions[] = {
            {"New", NewPath},
            {"Rect", MakeRect},
            {"Circle", MakeCircle},
            {"Ellipse", MakeEllipse},
            {"Polygon", MakePolygon},
            {"Star", MakeStar},
            {"Heart", MakeHeart},
            {"Line", MakeLine},
            {"Arc", MakeArc},
            {"Points", MakePoints},
            {"Svg", MakeSvg},
            {"Text", MakeText},
            {"TextGlyphs", MakeTextGlyphs},
            {nullptr, nullptr}};
    }

    VectorPath *TestPath(lua_State *L, int index)
    {
        return static_cast<VectorPath *>(luaL_testudata(L, index, PathMeta));
    }

    void PushPath(lua_State *L, const VectorPath &path)
    {
        void *memory = lua_newuserdatauv(L, sizeof(VectorPath), 0);
        new (memory) VectorPath(path);
        luaL_setmetatable(L, PathMeta);
    }

    void RegisterPath(lua_State *L)
    {
        luaL_newmetatable(L, PathMeta);
        luaL_newlib(L, PathMethods);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, PathGc);
        lua_setfield(L, -2, "__gc");
        lua_pushcfunction(L, PathToString);
        lua_setfield(L, -2, "__tostring");
        lua_pop(L, 1);

        luaL_newlib(L, PathFunctions);
        lua_setglobal(L, "Path");
    }
}
