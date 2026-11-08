#include "LuaComponents.h"

#include <Engine/Animation/Tween.h>
#include <Engine/Math/EntityUtility.h>
#include <Engine/Scripting/LuaHelpers.h>
#include <Engine/Scripting/LuaModules.h>
#include <Engine/Vector/VectorShape.h>

#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *VectorMeta = "SIMP.Vector";

        VectorShapeComponent *Check(lua_State *L)
        {
            return CheckComponent<VectorShapeComponent>(L, VectorMeta);
        }

        float Num(lua_State *L, int index)
        {
            return static_cast<float>(luaL_checknumber(L, index));
        }

        VectorPath *CheckPathArg(lua_State *L, int index)
        {
            VectorPath *path = TestPath(L, index);
            if (!path)
                luaL_error(L, "argument %d must be a Path (see Path.Circle, Path.Star, ...)", index);
            return path;
        }

        int VectorSetPath(lua_State *L)
        {
            VectorShapeComponent *v = Check(L);
            v->path = *CheckPathArg(L, 2);
            return 0;
        }

        int VectorGetPath(lua_State *L)
        {
            PushPath(L, Check(L)->path);
            return 1;
        }

        int VectorSetMorphTarget(lua_State *L)
        {
            VectorShapeComponent *v = Check(L);
            v->morphTarget = *CheckPathArg(L, 2);
            return 0;
        }

        int VectorClearMorphTarget(lua_State *L)
        {
            VectorShapeComponent *v = Check(L);
            v->morphTarget.Clear();
            v->morph = 0.0f;
            return 0;
        }

        int VectorGetMorph(lua_State *L)
        {
            lua_pushnumber(L, Check(L)->morph);
            return 1;
        }

        int VectorSetMorph(lua_State *L)
        {
            VectorShapeComponent *v = Check(L);
            v->morph = Num(L, 2);
            return 0;
        }

        int VectorMorphTo(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<VectorShapeComponent>(L, VectorMeta);
            VectorShapeComponent *v = &entity->GetComponent<VectorShapeComponent>();
            VectorPath *target = CheckPathArg(L, 2);
            const float duration = Num(L, 3);
            TweenSystem &system = entity->GetScene()->GetTweens();
            uint32_t id = 0;
            bool badCurve = false;
            {
                Curve curve;
                if (!ReadCurve(L, 4, curve))
                    badCurve = true;
                else
                {
                    if (v->morph != 0.0f && !v->morphTarget.Empty())
                    {
                        if (v->morph >= 1.0f)
                            v->path = v->morphTarget;
                        else
                        {
                            std::vector<FlatContour> outline;
                            VectorShapes::Outline(*v, 0.25f, outline);
                            VectorPath current;
                            for (const FlatContour &contour : outline)
                                current.Append(VectorPath::Points(contour.Points, contour.Closed));
                            v->path = std::move(current);
                        }
                    }
                    v->morphTarget = *target;
                    v->morph = 0.0f;
                    system.KillTarget(entity->GetHandle(), static_cast<int>(TweenProperty::Morph));
                    TweenTrack track;
                    track.Property = TweenProperty::Morph;
                    track.Channels = 1;
                    track.To[0] = 1.0f;
                    id = system.To(entity->GetHandle(), {track}, duration, curve);
                }
            }
            if (badCurve)
                return luaL_error(L, "MorphTo: unknown curve");
            ApplyTweenOptions(L, 5, system, id);
            return PushTween(L, system.Serial(), id);
        }

        int VectorSetCircle(lua_State *L)
        {
            VectorShapeComponent *v = Check(L);
            const float r = Num(L, 2);
            v->path = VectorPath::Circle(r);
            return 0;
        }

        int VectorSetRect(lua_State *L)
        {
            VectorShapeComponent *v = Check(L);
            const float w = Num(L, 2), h = Num(L, 3);
            const float r = static_cast<float>(luaL_optnumber(L, 4, 0.0));
            v->path = VectorPath::Rect(w, h, r);
            return 0;
        }

        int VectorSetPolygon(lua_State *L)
        {
            VectorShapeComponent *v = Check(L);
            const int sides = static_cast<int>(luaL_checkinteger(L, 2));
            const float r = Num(L, 3);
            v->path = VectorPath::Polygon(sides, r);
            return 0;
        }

        int VectorSetStar(lua_State *L)
        {
            VectorShapeComponent *v = Check(L);
            const int points = static_cast<int>(luaL_checkinteger(L, 2));
            const float outer = Num(L, 3);
            const float inner = static_cast<float>(luaL_optnumber(L, 4, outer * 0.45f));
            v->path = VectorPath::Star(points, outer, inner);
            return 0;
        }

        int VectorSetHeart(lua_State *L)
        {
            VectorShapeComponent *v = Check(L);
            const float size = Num(L, 2);
            v->path = VectorPath::Heart(size);
            return 0;
        }

        int VectorSetSvg(lua_State *L)
        {
            VectorShapeComponent *v = Check(L);
            size_t length = 0;
            const char *source = luaL_checklstring(L, 2, &length);
            const float size = static_cast<float>(luaL_optnumber(L, 3, 0.0));
            const bool applyStyle = lua_isnoneornil(L, 4) ? true : lua_toboolean(L, 4) != 0;
            bool ok = false;
            {
                const std::string text(source, length);
                VectorPath path;
                VectorStyleHint hint;
                const bool isFile = text.rfind("assets://", 0) == 0 || (text.size() > 4 && text.compare(text.size() - 4, 4, ".svg") == 0);
                if (isFile)
                    ok = path.LoadSvgFile(text, size, &hint);
                else if (text.find("<svg") != std::string::npos)
                    ok = path.LoadSvgText(text, size, &hint);
                else
                    ok = path.LoadSvgPathData(text, size);
                if (ok)
                {
                    v->path = std::move(path);
                    if (applyStyle && (hint.HasFill || hint.HasStroke))
                    {
                        v->fill = hint.HasFill;
                        if (hint.HasFill)
                            v->fillColor = hint.Fill;
                        v->stroke = hint.HasStroke;
                        if (hint.HasStroke)
                        {
                            v->strokeColor = hint.Stroke;
                            v->strokeWidth = hint.StrokeWidth;
                        }
                    }
                }
            }
            lua_pushboolean(L, ok);
            return 1;
        }

        int VectorSetFill(lua_State *L)
        {
            VectorShapeComponent *v = Check(L);
            ReadRgba(L, 2, v->fillColor);
            v->fill = true;
            return 0;
        }

        int VectorGetFill(lua_State *L)
        {
            return PushRgba(L, Check(L)->fillColor);
        }

        int VectorSetFillEnabled(lua_State *L)
        {
            Check(L)->fill = lua_toboolean(L, 2) != 0;
            return 0;
        }

        int VectorIsFillEnabled(lua_State *L)
        {
            lua_pushboolean(L, Check(L)->fill);
            return 1;
        }

        int VectorSetStroke(lua_State *L)
        {
            VectorShapeComponent *v = Check(L);
            ReadRgba(L, 2, v->strokeColor);
            v->stroke = true;
            return 0;
        }

        int VectorGetStroke(lua_State *L)
        {
            return PushRgba(L, Check(L)->strokeColor);
        }

        int VectorSetStrokeEnabled(lua_State *L)
        {
            Check(L)->stroke = lua_toboolean(L, 2) != 0;
            return 0;
        }

        int VectorIsStrokeEnabled(lua_State *L)
        {
            lua_pushboolean(L, Check(L)->stroke);
            return 1;
        }

        int VectorSetStrokeWidth(lua_State *L)
        {
            VectorShapeComponent *v = Check(L);
            v->strokeWidth = std::max(0.0f, Num(L, 2));
            return 0;
        }

        int VectorGetStrokeWidth(lua_State *L)
        {
            lua_pushnumber(L, Check(L)->strokeWidth);
            return 1;
        }

        int VectorSetCap(lua_State *L)
        {
            VectorShapeComponent *v = Check(L);
            const char *name = luaL_checkstring(L, 2);
            v->cap = std::strcmp(name, "butt") == 0 ? StrokeCap::Butt : std::strcmp(name, "square") == 0 ? StrokeCap::Square : StrokeCap::Round;
            return 0;
        }

        int VectorSetJoin(lua_State *L)
        {
            VectorShapeComponent *v = Check(L);
            const char *name = luaL_checkstring(L, 2);
            v->join = std::strcmp(name, "miter") == 0 ? StrokeJoin::Miter : std::strcmp(name, "bevel") == 0 ? StrokeJoin::Bevel : StrokeJoin::Round;
            v->miterLimit = static_cast<float>(luaL_optnumber(L, 3, v->miterLimit));
            return 0;
        }

        int VectorSetTrim(lua_State *L)
        {
            VectorShapeComponent *v = Check(L);
            v->trimStart = Num(L, 2);
            v->trimEnd = Num(L, 3);
            v->trimOffset = static_cast<float>(luaL_optnumber(L, 4, 0.0));
            return 0;
        }

        int VectorGetTrim(lua_State *L)
        {
            const VectorShapeComponent *v = Check(L);
            lua_pushnumber(L, v->trimStart);
            lua_pushnumber(L, v->trimEnd);
            lua_pushnumber(L, v->trimOffset);
            return 3;
        }

        int VectorSetDash(lua_State *L)
        {
            VectorShapeComponent *v = Check(L);
            v->dashLength = std::max(0.0f, Num(L, 2));
            v->dashGap = std::max(0.0f, static_cast<float>(luaL_optnumber(L, 3, v->dashLength)));
            v->dashOffset = static_cast<float>(luaL_optnumber(L, 4, 0.0));
            return 0;
        }

        int VectorGetDash(lua_State *L)
        {
            const VectorShapeComponent *v = Check(L);
            lua_pushnumber(L, v->dashLength);
            lua_pushnumber(L, v->dashGap);
            lua_pushnumber(L, v->dashOffset);
            return 3;
        }

        int VectorSetGradient(lua_State *L)
        {
            VectorShapeComponent *v = Check(L);
            const float x1 = Num(L, 2), y1 = Num(L, 3), x2 = Num(L, 4), y2 = Num(L, 5);
            SDL_Color a{ToByte(L, 6), ToByte(L, 7), ToByte(L, 8), ToByte(L, 9)};
            SDL_Color b{ToByte(L, 10), ToByte(L, 11), ToByte(L, 12), ToByte(L, 13)};
            v->gradient = true;
            v->fill = true;
            v->gradientFrom = {x1, y1};
            v->gradientTo = {x2, y2};
            v->gradientFromColor = a;
            v->gradientToColor = b;
            return 0;
        }

        int VectorClearGradient(lua_State *L)
        {
            Check(L)->gradient = false;
            return 0;
        }

        int VectorGetLength(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<VectorShapeComponent>(L, VectorMeta);
            const WorldTransform2D world = DecomposeWorld(entity->GetComponent<TransformComponent>());
            lua_pushnumber(L, VectorShapes::WorldLength(entity->GetComponent<VectorShapeComponent>(), world.Scale));
            return 1;
        }

        int VectorGetPointAt(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<VectorShapeComponent>(L, VectorMeta);
            const float t = Num(L, 2);
            const WorldTransform2D world = DecomposeWorld(entity->GetComponent<TransformComponent>());
            bool ok = false;
            glm::vec2 point(0.0f), tangent(1.0f, 0.0f);
            {
                std::vector<FlatContour> outline;
                VectorShapes::Outline(entity->GetComponent<VectorShapeComponent>(), 0.25f, outline);
                if (!outline.empty())
                    ok = PointAtDistance(outline[0].Points, outline[0].Closed, t * PolylineLength(outline[0].Points, outline[0].Closed), point, tangent);
            }
            if (!ok)
                return 0;
            const float r = glm::radians(world.Rotation);
            const float c = std::cos(r), s = std::sin(r);
            const glm::vec2 q = point * world.Scale;
            const glm::vec2 p = world.Position + glm::vec2(q.x * c - q.y * s, q.x * s + q.y * c);
            const glm::vec2 tw = glm::vec2(tangent.x * world.Scale.x, tangent.y * world.Scale.y);
            lua_pushnumber(L, p.x);
            lua_pushnumber(L, p.y);
            lua_pushnumber(L, glm::degrees(std::atan2(tw.y, tw.x)) + world.Rotation);
            return 3;
        }

        int VectorGetBounds(lua_State *L)
        {
            glm::vec2 min, max;
            if (!Check(L)->path.GetBounds(min, max))
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

        const luaL_Reg VectorMethods[] = {
            {"SetPath", VectorSetPath},
            {"GetPath", VectorGetPath},
            {"SetMorphTarget", VectorSetMorphTarget},
            {"ClearMorphTarget", VectorClearMorphTarget},
            {"GetMorph", VectorGetMorph},
            {"SetMorph", VectorSetMorph},
            {"MorphTo", VectorMorphTo},
            {"SetCircle", VectorSetCircle},
            {"SetRect", VectorSetRect},
            {"SetPolygon", VectorSetPolygon},
            {"SetStar", VectorSetStar},
            {"SetHeart", VectorSetHeart},
            {"SetSvg", VectorSetSvg},
            {"SetFill", VectorSetFill},
            {"GetFill", VectorGetFill},
            {"SetFillEnabled", VectorSetFillEnabled},
            {"IsFillEnabled", VectorIsFillEnabled},
            {"SetStroke", VectorSetStroke},
            {"GetStroke", VectorGetStroke},
            {"SetStrokeEnabled", VectorSetStrokeEnabled},
            {"IsStrokeEnabled", VectorIsStrokeEnabled},
            {"SetStrokeWidth", VectorSetStrokeWidth},
            {"GetStrokeWidth", VectorGetStrokeWidth},
            {"SetCap", VectorSetCap},
            {"SetJoin", VectorSetJoin},
            {"SetTrim", VectorSetTrim},
            {"GetTrim", VectorGetTrim},
            {"SetDash", VectorSetDash},
            {"GetDash", VectorGetDash},
            {"SetGradient", VectorSetGradient},
            {"ClearGradient", VectorClearGradient},
            {"GetLength", VectorGetLength},
            {"GetPointAt", VectorGetPointAt},
            {"GetBounds", VectorGetBounds},
            {nullptr, nullptr}};
    }

    const ComponentBinding &VectorBinding()
    {
        static const ComponentBinding binding{"Vector", VectorMeta, VectorMethods, &HasOf<VectorShapeComponent>, &AddOf<VectorShapeComponent>, &RemoveOf<VectorShapeComponent>};
        return binding;
    }
}
