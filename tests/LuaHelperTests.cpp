#include "TestFramework.h"

#include <Engine/Core/VFS.h>
#include <Engine/Scripting/LuaRuntime.h>

#include <string>

using namespace SIMPEngine;

namespace
{
    const char *Prelude = "local function near(a, b) return math.abs(a - b) < 1e-4 end\n";

    bool Run(const std::string &code)
    {
        return LuaRuntime::Get().ExecuteString(std::string(Prelude) + code, "helper test");
    }
}

TEST_CASE("lua helpers: Math.Progress, Window, Remap, Smoothstep and Clamp01")
{
    CHECK(Run(R"(
        assert(near(Math.Progress(5, 0, 10), 0.5))
        assert(Math.Progress(-3, 0, 10) == 0 and Math.Progress(30, 0, 10) == 1)
        assert(Math.Progress(2, 4, 4) == 0 and Math.Progress(4, 4, 4) == 1)
        assert(near(Math.Progress(7.5, 10, 5), 0.5))
        assert(Math.Clamp01(-1) == 0 and Math.Clamp01(2) == 1 and near(Math.Clamp01(0.25), 0.25))
        assert(near(Math.Remap(5, 0, 10, 100, 200), 150))
        assert(near(Math.Remap(15, 0, 10, 100, 200), 250))
        assert(near(Math.Remap(15, 0, 10, 100, 200, true), 200))
        assert(near(Math.Smoothstep(0, 1, 0.5), 0.5) and Math.Smoothstep(0, 1, 2) == 1 and Math.Smoothstep(2, 4, 0) == 0)
        assert(near(Math.Window(0, 1, 5, 0.5), 0) and near(Math.Window(1.25, 1, 5, 0.5), 0.5))
        assert(near(Math.Window(3, 1, 5, 0.5), 1) and near(Math.Window(4.75, 1, 5, 0.5), 0.5) and near(Math.Window(6, 1, 5, 0.5), 0))
    )"));
}

TEST_CASE("lua helpers: Color converts hex strings and tables, mixes and unpacks")
{
    CHECK(Run(R"(
        local function same(c, r, g, b, a) return c[1] == r and c[2] == g and c[3] == b and c[4] == a end
        assert(same(Color.Hex("#58ceff"), 88, 206, 255, 255))
        assert(same(Color.Hex("58ceff"), 88, 206, 255, 255))
        assert(same(Color.Hex("#fa0"), 255, 170, 0, 255))
        assert(same(Color.Hex("#11223380"), 17, 34, 51, 128))
        assert(same(Color.Hex("#f008"), 255, 0, 0, 136))
        assert(same(Color.Hex("#58ceff", 0.5), 88, 206, 255, 128))
        assert(same(Color.New(300, -5, 10.4), 255, 0, 10, 255))
        assert(same(Color.Mix("#000000", "#ffffff", 0.5), 128, 128, 128, 255))
        assert(same(Color.Mix({0, 0, 0, 0}, {255, 100, 50, 255}, 1), 255, 100, 50, 255))
        assert(same(Color.Mix("#102030", "#102030", 0.3), 16, 32, 48, 255))
        assert(same(Color.WithAlpha("#ff0000", 0.25), 255, 0, 0, 64))
        local r, g, b, a = Color.Unpack("#336699")
        assert(r == 51 and g == 102 and b == 153 and a == 255)
        local _, _, _, a2 = Color.Unpack({10, 20, 30}, 0.5)
        assert(a2 == 128)
    )"));
    CHECK(!Run("Color.Hex('#12')"));
    CHECK(!Run("Color.Hex('nope!')"));
    CHECK(!Run("Color.Mix(5, '#fff', 0.5)"));
}

TEST_CASE("lua helpers: Timeline segments, progress and lookup")
{
    CHECK(Run(R"(
        local tl = Timeline.New({{"intro", 4.6}, {"pan", 2.4}})
        tl:Add("axes", 2.2):Add("curve", 5.4)
        local a, b = tl:Span("curve")
        assert(near(a, 9.2) and near(b, 14.6))
        assert(near(tl:Total(), 14.6))
        assert(near(tl:Progress(10.9, "curve"), 0.3148148) and tl:Progress(0, "curve") == 0 and tl:Progress(20, "curve") == 1)
        assert(tl:Active(9.2, "curve") and not tl:Active(14.6, "curve") and not tl:Active(9.1, "curve"))
        local name, p = tl:Current(5)
        assert(name == "pan" and near(p, (5 - 4.6) / 2.4))
        name, p = tl:Current(99)
        assert(name == "curve" and p == 1)
        assert(Timeline.New():Current(1) == nil)
    )"));
    CHECK(!Run("Timeline.New():Span('missing')"));
    CHECK(!Run("Timeline.New():Add('a', 1):Add('a', 2)"));
    CHECK(!Run("Timeline.New():Add('a', -1)"));
}

TEST_CASE("lua helpers: Ease wraps the engine curves by short or full name")
{
    CHECK(Run(R"(
        for _, name in ipairs({"inOutCubic", "easeInOutCubic", "outBack", "inOutSine", "linear", "bounceOut"}) do
            local fn = Ease[name]
            assert(type(fn) == "function")
            local full = name:sub(1, 4) == "ease" and name or (name == "linear" or name == "bounceOut") and name or "ease" .. name:sub(1, 1):upper() .. name:sub(2)
            assert(near(fn(0.3), Curves.Evaluate(full, 0.3)), name)
            assert(near(fn(0), 0) and near(fn(1), 1), name)
        end
        assert(Ease.inOutCubic == Ease.inOutCubic)
        assert(rawget(Ease, "inOutCubic") == nil)
    )"));
    CHECK(!Run("local x = Ease.notACurve"));
}

TEST_CASE("lua helpers: Draw.MeasureText reports a size without a draw pass")
{
    VFS::Mount("engine", SIMP_TEST_ASSETS);
    CHECK(Run(R"(
        local w1, h1 = Draw.MeasureText("Hello", 40)
        local w2 = Draw.MeasureText("Hello Hello", 40)
        local w3 = Draw.MeasureText("Hello", 80)
        assert(w1 > 20 and h1 > 20)
        assert(w2 > w1 * 1.8 and w2 < w1 * 2.4)
        assert(near(w3, w1 * 2))
        local _, h2 = Draw.MeasureText("a\nb", 40)
        assert(near(h2, h1 * 2))
        local zw, zh = Draw.MeasureText("", 40)
        assert(zw == 0)
        assert(select("#", Draw.MeasureText("x", 10)) == 2)
    )"));
    CHECK(Run(R"(
        Draw.SetZ(3)
        Draw.Layer(7, function() assert(Draw.GetZ() == 7) end)
        assert(Draw.GetZ() == 3)
        local ok, err = pcall(Draw.Layer, 9, function() error("boom", 0) end)
        assert(not ok and err == "boom" and Draw.GetZ() == 3)
        Draw.SetZ(0)
    )"));
    CHECK(Run("assert(type(Draw.Layer) == 'function' and type(Draw.Dashed) == 'function' and type(Draw.Arrow) == 'function')"));
    VFS::UnMount("engine");
}

TEST_CASE("lua helpers: CameraRig moves between poses along the timeline")
{
    CHECK(Run(R"(
        local tl = Timeline.New({{"a", 2}, {"b", 2}, {"c", 2}})
        local rig = CameraRig.New(tl, {0, 0, 1})
        rig:Move("a", {100, 0, 2}, "linear")
        rig:Move("b", {100, 50, 2}, "linear", {bump = 0.5})
        rig:Track("c", function(u) return 100 - 100 * u, 50, 2 end)
        local x, y, z = rig:At(0)
        assert(near(x, 0) and near(y, 0) and near(z, 1))
        x, y, z = rig:At(1)
        assert(near(x, 50) and near(y, 0) and near(z, 1.5))
        x, y, z = rig:At(2)
        assert(near(x, 100) and near(y, 0) and near(z, 2))
        x, y, z = rig:At(3)
        assert(near(x, 100) and near(y, 25) and near(z, 2.5), x .. " " .. y .. " " .. z)
        x, y, z = rig:At(4)
        assert(near(x, 100) and near(y, 50) and near(z, 2))
        x, y, z = rig:At(5)
        assert(near(x, 50) and near(y, 50) and near(z, 2))
        x, y, z = rig:At(99)
        assert(near(x, 0) and near(y, 50) and near(z, 2))
        x, y, z = rig:At(-5)
        assert(near(x, 0) and near(z, 1))
        local swayed = CameraRig.New(tl, {0, 0, 1}):Sway(5, 4, 0.004)
        x, y, z = swayed:At(1)
        assert(math.abs(x) > 1 and math.abs(y) > 1 and z ~= 1)
        local fake = {IsValid = function() return true end}
        assert(rig:Apply(1, nil) == 50)
    )"));
    CHECK(!Run("local tl = Timeline.New({{'a', 1}}); CameraRig.New(tl):Move('missing', {0, 0, 1})"));
}

TEST_CASE("lua helpers: Captions fade in and out inside their segments")
{
    CHECK(Run(R"(
        local tl = Timeline.New({{"a", 2}, {"b", 2}, {"c", 2}})
        local caps = Captions.New(tl)
        caps:Add("a", "b", "STEP 1", "First text")
        caps:Add("c", "c", "STEP 2")
        assert(#caps:Visible(0) == 0 and #caps:Visible(0.25) == 0)
        local v = caps:Visible(0.475)
        assert(#v == 1 and near(v[1].alpha, 0.5) and v[1].title == "STEP 1" and v[1].text == "First text")
        assert(near(caps:Visible(2)[1].alpha, 1))
        assert(near(caps:Visible(3.525)[1].alpha, 0.5))
        assert(#caps:Visible(3.9) == 0)
        local both = caps:Visible(4.6)
        assert(#both == 1 and both[1].title == "STEP 2" and both[1].text == "")
        assert(#caps:Visible(6) == 0)
    )"));
    CHECK(!Run("Captions.New(Timeline.New()):Add('x', 'y', 'T')"));
}

TEST_CASE("lua helpers: colors accept palette names and custom names")
{
    CHECK(Run(R"LUA(
        local function same(c, r, g, b) return c[1] == r and c[2] == g and c[3] == b end
        assert(same(Color.Get("cyan"), 88, 206, 255))
        assert(same(Color.Get("gold"), 255, 206, 96))
        assert(same(Color.Mix("black", "white", 0.5), 128, 128, 128))
        assert(same(Color.WithAlpha("coral", 0.5), 255, 126, 96) and Color.WithAlpha("coral", 0.5)[4] == 128)
        local brand = Color.Define("brand", "#123456")
        assert(same(brand, 18, 52, 86) and same(Color.Get("brand"), 18, 52, 86))
        local r, g, b = Color.Unpack("brand")
        assert(r == 18 and g == 52 and b == 86)
        local found = false
        for _, name in ipairs(Color.Names()) do
            if name == "brand" then found = true end
        end
        assert(found and #Color.Names() >= 17)
        assert(same(Color.Define("cyan", "#000000"), 0, 0, 0))
        Color.Define("cyan", "#58ceff")
    )LUA"));
    CHECK(!Run("Color.Get('cyn')LUA"));
    CHECK(!Run("Color.Define('#bad', '#fff')LUA"));
    CHECK(!Run("Color.Define('', '#fff')LUA"));
}

TEST_CASE("lua helpers: unknown names suggest the closest match")
{
    CHECK(Run(R"LUA(
        assert(Suggest.Closest("popin", {"fadeIn", "popIn", "popOut"}) == "popIn")
        assert(Suggest.Closest("zzzzzz", {"fadeIn", "popIn"}) == nil)
        assert(Suggest.Hint("popin", {"popIn"}) == " (did you mean 'popIn'?)")
        assert(Suggest.Hint("zzzzzz", {"popIn"}) == "")
        local ok, err = pcall(function() return Effect.PopUp end)
        assert(not ok and err:find("did you mean"), err)
        ok, err = pcall(function() return Ease.inoutcubik end)
        assert(not ok and err:find("did you mean"), err)
        local tl = Timeline.New({{"intro", 1}, {"curve", 2}})
        ok, err = pcall(tl.Span, tl, "curv")
        assert(not ok and err:find("did you mean 'curve'"), err)
    )LUA"));
}

TEST_CASE("lua helpers: Timeline Window and Eased, Math.Integrate")
{
    CHECK(Run(R"LUA(
        local tl = Timeline.New({{"a", 2}, {"b", 4}})
        assert(near(tl:Window(2.2, "b", 0.4), 0.5) and near(tl:Window(4, "b", 0.4), 1) and near(tl:Window(5.8, "b", 0.4), 0.5) and tl:Window(1, "b") == 0)
        assert(near(tl:Eased(4, "b"), 0.5))
        assert(near(tl:Eased(4, "b", function(u) return u * u end), 0.25))
        assert(near(tl:Eased(4, "b", "linear"), 0.5))
        assert(near(Math.Integrate(function(x) return x * x end, 0, 3), 9))
        assert(near(Math.Integrate(math.sin, 0, math.pi), 2))
        assert(near(Math.Integrate(function(x) return 1 end, 2, 5, 3), 3))
    )LUA"));
}

TEST_CASE("lua helpers: Axes, Plot, Area and Riemann compute their geometry")
{
    CHECK(Run(R"LUA(
        local ax = Axes.New{origin = {-675, -330}, scale = {270, 180}, x = {-0.4, 5.55, 1}, y = {-0.111, 3.95, 1}}
        local px, py = ax:P(5, 2)
        assert(near(px, 675) and near(py, 30))
        assert(#ax.xticks == 6 and ax.xticks[1] == 0 and ax.xticks[6] == 5)
        assert(#ax.yticks == 3 and ax.yticks[1] == 1 and ax.yticks[3] == 3)
        local explicit = Axes.New{origin = {0, 0}, scale = {1, 1}, x = {0, 4}, y = {0, 4}, xticks = {1, 4}, yticks = {}}
        assert(#explicit.xticks == 2 and #explicit.yticks == 0)

        local line = Axes.New{origin = {0, 0}, scale = {100, 50}, x = {0, 1, 0.5}, y = {0, 1, 0.5}}
        local plot = line:Plot(function(x) return 2 * x end, 0, 1, {samples = 11})
        assert(#plot:Points() == 22)
        local pts = plot:Points(0.5)
        assert(#pts == 12 and near(pts[11], 50) and near(pts[12], 50))
        pts = plot:Points(0.55)
        assert(#pts == 14 and near(pts[13], 55) and near(pts[14], 55))
        assert(plot:Points() == plot:Points(1) and #plot:Points(0) == 2)
        local ax2, ay2 = plot:At(0.25)
        assert(near(ax2, 25) and near(ay2, 25) and near(plot:Value(0.25), 0.5))

        local area = line:Area(function(x) return x end, 0, 1, {samples = 5})
        local poly = area:Polygon()
        assert(#poly == 2 + 10 + 2 and poly[1] == 0 and poly[2] == 0 and near(poly[#poly - 1], 100) and poly[#poly] == 0)
        assert(#area:Polygon(0.5) == 2 + 6 + 2)

        local rz = line:Riemann(function(x) return x end, 0, 1)
        assert(near(rz:Sum(4), 0.375))
        rz.mode = "right"
        assert(near(rz:Sum(4), 0.625))
        rz.mode = "mid"
        assert(near(rz:Sum(4), 0.5))
        rz.mode = "left"
        local rects = rz:Rects(4)
        assert(#rects == 3, #rects)
        assert(near(rects[1].x0, 25) and near(rects[1].x1, 50) and near(rects[3].h, 0.75 * 50) and near(rects[1].t, 0.375))
        rects = rz:Rects(4, {window = {0.5, 1}})
        assert(#rects == 2 and rects[1].index == 3 and rects[2].index == 4, #rects)
        local grown = rz:Rects(4, {grow = function(i) return 0.5 end})
        assert(near(grown[1].h, 0.25 * 50 * 0.5))
        local morph = rz:Rects(4, {from = 2, u = 0})
        assert(#morph == 2 and morph[1].index == 3 and near(morph[1].h, 25) and near(morph[2].h, 25))
        local settled = rz:Rects(4, {from = 2, u = 1})
        assert(#settled == 3 and near(settled[3].h, 37.5))
        assert(near(Math.Integrate(function(x) return x end, 0, 1), 0.5))
    )LUA"));
}
