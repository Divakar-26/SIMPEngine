#include "TestFramework.h"

#include <Engine/Animation/MotionPresets.h>
#include <Engine/Core/TimeStep.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scripting/LuaBindings.h>
#include <Engine/Scripting/LuaRuntime.h>

#include <set>
#include <string>

using namespace SIMPEngine;

TEST_CASE("motion presets: every preset builds, animates and settles")
{
    const auto &all = MotionPresets::All();
    CHECK(all.size() >= 100);
    std::set<std::string> names;
    for (const MotionPreset &preset : all)
    {
        CHECK(names.insert(preset.Name).second);
        Scene scene("Motion");
        float length = 0.0f;
        const entt::entity root = MotionPresets::Play(scene, preset, {}, &length);
        CHECK(scene.GetRegistry().valid(root));
        CHECK(length > 0.0f);
        const size_t entities = scene.GetRegistry().view<TransformComponent>().size();
        if (entities < 2)
            std::printf("    preset %s spawned nothing\n", preset.Name.c_str());
        CHECK(entities >= 2);
        for (float t = 0.0f; t < length + 0.5f; t += 1.0f / 30.0f)
            scene.GetTweens().Update(1.0f / 30.0f);
        scene.OnUpdate(0.016f);
    }
    CHECK(MotionPresets::Find("circlepop") != nullptr);
    CHECK(MotionPresets::Find("NoSuchPreset") == nullptr);
}

TEST_CASE("motion presets: options scale time, take points and remove after hold")
{
    const MotionPreset *arrow = MotionPresets::Find("ArrowSlide");
    CHECK(arrow != nullptr);
    Scene scene("Motion");
    MotionOptions options;
    options.Points = {{0.0f, 0.0f}, {100.0f, 0.0f}, {100.0f, 100.0f}};
    options.Duration = arrow->Duration * 2.0f;
    options.Hold = 0.0f;
    float fast = 0.0f, slow = 0.0f;
    {
        Scene other("Other");
        MotionPresets::Play(other, *arrow, {}, &fast);
    }
    const entt::entity root = MotionPresets::Play(scene, *arrow, options, &slow);
    CHECK_NEAR(slow, fast * 2.0f, 0.01f);
    CHECK(scene.GetRegistry().all_of<LifetimeComponent>(root));
    for (int i = 0; i < 200 && scene.GetRegistry().valid(root); ++i)
        scene.OnUpdate(0.05f);
    CHECK(!scene.GetRegistry().valid(root));
}

TEST_CASE("motion presets: Lua Motion API")
{
    Scene scene("Lua Motion");
    LuaBindings::SceneScope scope(&scene);
    CHECK(LuaRuntime::Get().ExecuteString(R"(
        assert(#Motion.Names() >= 100)
        assert(Motion.Info("CirclePop").category == "Shapes")
        local a = Motion.CirclePop(10, 20, {size = 30, color = "#ff0000"})
        assert(a:IsValid())
        local b = Motion.Arrow({{0, 0}, {100, 0}, {100, 80}}, {width = 4})
        local c = Motion.Plot(function(x) return x * x end, {range = {-2, 2}})
        local d = Motion.BarChart({3, 1, 4}, {x = 50})
        local e = Motion.Typewriter("Hi there")
        local f = Motion.Play("Confetti", {x = 0, y = 0})
        assert(b:IsValid() and c:IsValid() and d:IsValid() and e:IsValid() and f:IsValid())
        local ok = pcall(function() return Motion.NoSuchThing(0, 0) end)
        assert(not ok)
    )", "motion test"));
}

TEST_CASE("agent friendly API: spawn spec, colors everywhere, helpful errors")
{
    Scene scene("Spawn");
    LuaBindings::SceneScope scope(&scene);
    CHECK(LuaRuntime::Get().ExecuteString(R"(
        local coin = Scene.Spawn{name = "Coin", x = 10, y = 20, z = 2, shape = "circle", size = 12, color = "#ffd166", glow = 1.5}
        assert(coin:GetName() == "Coin" and coin:Has("Vector") and coin:Has("Glow"))
        local x, y = coin:GetPosition()
        assert(x == 10 and y == 20)
        local r, g, b = coin:GetVector():GetFill()
        assert(r == 255 and g == 209 and b == 102)
        local box = Scene.Spawn{shape = "rect", w = 80, h = 20, color = {40, 50, 60}, body = "static"}
        assert(box:Has("Body") and box:GetBody():IsStatic())
        local ball = Scene.Spawn{shape = "circle", radius = 10, body = "dynamic", bounce = 0.5}
        assert(ball:Has("Body") and not ball:GetBody():IsStatic())
        local label = Scene.Spawn{text = "Score", fontSize = 40, color = "red"}
        assert(label:GetText():GetFontSize() == 40)
        local plain = Scene.Spawn("Plain", 1, 2)
        assert(plain:GetName() == "Plain")
        local line = Scene.Spawn{shape = "line", points = {{0, 0}, {100, 0}}, color = "#ffffff", strokeWidth = 6}
        assert(line:GetVector():GetStrokeWidth() == 6)

        local sprite = Scene.Spawn("S"):Add("Sprite")
        sprite:SetTint("#102030")
        local tr, tg, tb = sprite:GetTint()
        assert(tr == 16 and tg == 32 and tb == 48)
        sprite:SetTint({1, 2, 3, 4})
        local _, _, _, ta = sprite:GetTint()
        assert(ta == 4)
        sprite:SetTint(9, 8, 7)
        assert(select(1, sprite:GetTint()) == 9)
        Scene.SetAmbient("#808080")

        local ok, err = pcall(function() coin:Add("Sprit") end)
        assert(not ok and err:find("did you mean 'Sprite'"), err)
        ok, err = pcall(function() Tween.To(coin, {alpah = 0}, 1) end)
        assert(not ok and err:find("did you mean 'alpha'"), err)
        ok, err = pcall(function() Scene.Spawn{shap = "circle"} end)
        assert(not ok and err:find("did you mean 'shape'"), err)
        ok, err = pcall(function() Scene.Spawn{shape = "circel"} end)
        assert(not ok and err:find("circle"), err)
        ok, err = pcall(function() sprite:SetTint("nope") end)
        assert(not ok and err:find("bad color"), err)
    )", "agent api test"));
}

TEST_CASE("pocket puzzles gaps: integer Game.Get, text variables, Vector:Contains, Text font and measure")
{
    Scene scene("Gaps");
    LuaBindings::SceneScope scope(&scene);
    CHECK(LuaRuntime::Get().ExecuteString(R"(
        Game.Set("score", 12)
        assert(math.type(Game.Get("score", 0)) == "integer" and Game.Get("score", 0) == 12)
        assert(math.type(Game.Get("ratio", 0.5)) == "float")
        assert(math.type(Game.Add("score", 3)) == "integer" and Game.Get("score") == 15)
        Game.SetText("player", "Mira")
        assert(Game.GetText("player") == "Mira" and Game.HasText("player") and Game.GetText("none", "x") == "x")
        Game.Save("gaps_test")
        Game.SetText("player", "Other")
        Game.Set("score", 1)
        assert(Game.Load("gaps_test"))
        assert(Game.GetText("player") == "Mira" and Game.Get("score") == 15)
        Game.DeleteSave("gaps_test")

        local c = Scene.Spawn{shape = "circle", size = 20, x = 100, y = 50}
        local v = c:GetVector()
        assert(v:Contains(100, 50) and v:Contains(115, 50) and not v:Contains(130, 50) and not v:Contains(0, 0))
        local r = Scene.Spawn{shape = "rect", w = 40, h = 20, x = -10, y = -10}
        assert(r:GetVector():Contains(-10, -10) and not r:GetVector():Contains(-10, 10))

        local label = Scene.Spawn{text = "Hello", fontSize = 40}
        local t = label:GetText()
        t:SetFont("")
        assert(t:GetFont() == "")
        local w, h = t:Measure()
        assert(type(w) == "number" and type(h) == "number")
        local vw, vh = Window.GetViewSize()
        assert(type(vw) == "number" and type(vh) == "number")
    )", "gaps test"));
}

TEST_CASE("vector shadow, fixed stroke, pointer helper, per-corner rect, shake on vector")
{
    Scene scene("Extras");
    LuaBindings::SceneScope scope(&scene);
    CHECK(LuaRuntime::Get().ExecuteString(R"(
        local c = Scene.Spawn{shape = "circle", size = 20, x = 10, y = 10}
        local v = c:GetVector()
        v:SetShadow("#000000", 2, -3, 6)
        v:SetFixedStroke(true)
        v:ClearShadow()
        local pointer = Pointer.New(c)
        pointer:Update(0.016)
        assert(pointer.down == false and pointer.tap == false and pointer:Inside(0, 0, 10000, 10000))
        pointer:Capture("a")
        assert(pointer:IsCaptured("a"))
        pointer:Release()
        assert(not pointer:IsCaptured("a"))
        local p = Path.Rect(100, 50, {10, 0, 20, 0})
        assert(p ~= nil)
        Effect.Shake(c)
    )", "extras test"));
    float startX = 0.0f;
    for (auto e : scene.GetRegistry().view<VectorShapeComponent, TransformComponent>())
        startX = scene.GetRegistry().get<TransformComponent>(e).position.x;
    float moved = 0.0f;
    for (int i = 0; i < 5; ++i)
    {
        scene.OnUpdate(0.02f);
        for (auto e : scene.GetRegistry().view<VectorShapeComponent, TransformComponent>())
            moved = std::max(moved, std::abs(scene.GetRegistry().get<TransformComponent>(e).position.x - startX));
    }
    CHECK(moved > 0.5f);
}
