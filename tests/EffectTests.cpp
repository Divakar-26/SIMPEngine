#include "TestFramework.h"

#include <Engine/Animation/EffectPresets.h>
#include <Engine/Core/GameVariables.h>
#include <Engine/Core/VFS.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>

using namespace SIMPEngine;

namespace
{
    struct State
    {
        float x, y, sx, sy, rotation, alpha;
    };

    Entity MakeSubject(Scene &scene)
    {
        Entity e = scene.CreateEntity("Subject");
        auto &t = e.GetComponent<TransformComponent>();
        t.position = {50.0f, 30.0f};
        t.scale = {1.0f, 1.0f};
        t.rotation = 0.0f;
        e.AddComponent<RenderComponent>();
        return e;
    }

    State Read(Entity e)
    {
        const auto &t = e.GetComponent<TransformComponent>();
        return {t.position.x, t.position.y, t.scale.x, t.scale.y, t.rotation, static_cast<float>(e.GetComponent<RenderComponent>().color.a)};
    }

    bool AtRest(const State &s)
    {
        return std::fabs(s.x - 50.0f) < 0.05f && std::fabs(s.y - 30.0f) < 0.05f && std::fabs(s.sx - 1.0f) < 0.005f && std::fabs(s.sy - 1.0f) < 0.005f &&
               std::fabs(std::remainder(s.rotation, 360.0f)) < 0.05f && s.alpha == 255.0f;
    }

    float Deviation(const State &s)
    {
        return std::fabs(s.x - 50.0f) + std::fabs(s.y - 30.0f) + std::fabs(s.sx - 1.0f) * 100.0f + std::fabs(s.sy - 1.0f) * 100.0f + std::fabs(s.rotation) + (255.0f - s.alpha);
    }

    void Advance(Scene &scene, float seconds, float *maxDeviation = nullptr, Entity watched = {})
    {
        const int steps = static_cast<int>(std::ceil(seconds * 60.0f));
        for (int i = 0; i < steps; ++i)
        {
            scene.GetTweens().Update(1.0f / 60.0f);
            if (maxDeviation && watched)
                *maxDeviation = std::max(*maxDeviation, Deviation(Read(watched)));
        }
    }
}

TEST_CASE("effects: the catalogue is unique, categorised and built from consistent segments")
{
    const auto &all = EffectPresets::All();
    CHECK(all.size() >= 36);
    std::set<std::string> names;
    for (const EffectPreset &p : all)
    {
        CHECK(names.insert(p.Name).second);
        CHECK(!p.Label.empty() && !p.Description.empty() && p.Duration > 0.0f);
        CHECK(p.Category == "Entrance" || p.Category == "Exit" || p.Category == "Attention" || p.Category == "Loop");
        CHECK(!p.Lanes.empty());
        for (const EffectLane &lane : p.Lanes)
        {
            CHECK(!lane.Segments.empty());
            for (const EffectSegment &segment : lane.Segments)
            {
                CHECK(segment.Length > 0.0f);
                bool from = !segment.Tracks.empty() && segment.Tracks.front().HasFrom;
                for (const EffectTrack &track : segment.Tracks)
                    CHECK(track.HasFrom == from);
            }
        }
        CHECK(EffectPresets::Find(p.Name) == &p);
    }
    CHECK(EffectPresets::Categories().size() == 4);
    CHECK(EffectPresets::Find("nope") == nullptr);
}

TEST_CASE("effects: entrances start away from rest and end exactly at rest")
{
    for (const EffectPreset &p : EffectPresets::All())
    {
        if (p.Category != "Entrance")
            continue;
        Scene scene("Effect");
        Entity e = MakeSubject(scene);
        const uint32_t id = EffectPresets::Play(scene.GetTweens(), e.GetHandle(), p);
        CHECK(id != 0);
        scene.GetTweens().Update(0.001f);
        CHECK(Deviation(Read(e)) > 5.0f);
        Advance(scene, p.Duration + 0.2f);
        CHECK(AtRest(Read(e)));
        CHECK(!scene.GetTweens().IsRunning(id));
    }
}

TEST_CASE("effects: exits end invisible")
{
    for (const EffectPreset &p : EffectPresets::All())
    {
        if (p.Category != "Exit")
            continue;
        Scene scene("Effect");
        Entity e = MakeSubject(scene);
        EffectPresets::Play(scene.GetTweens(), e.GetHandle(), p);
        Advance(scene, p.Duration + 0.2f);
        CHECK(Read(e).alpha == 0.0f);
    }
}

TEST_CASE("effects: attention effects move and come back to rest")
{
    for (const EffectPreset &p : EffectPresets::All())
    {
        if (p.Category != "Attention")
            continue;
        Scene scene("Effect");
        Entity e = MakeSubject(scene);
        const uint32_t id = EffectPresets::Play(scene.GetTweens(), e.GetHandle(), p);
        float peak = 0.0f;
        Advance(scene, p.Duration + 0.2f, &peak, e);
        if (peak <= 2.0f)
            SIMPTest::Fail(__FILE__, __LINE__, p.Name + " barely moves (peak " + std::to_string(peak) + ")");
        if (!AtRest(Read(e)))
        {
            const State r = Read(e);
            SIMPTest::Fail(__FILE__, __LINE__, p.Name + " does not return to rest: x " + std::to_string(r.x) + " y " + std::to_string(r.y) + " scale " + std::to_string(r.sx) + "," + std::to_string(r.sy) + " rot " + std::to_string(r.rotation) + " alpha " + std::to_string(r.alpha));
        }
        CHECK(!scene.GetTweens().IsRunning(id));
    }
}

TEST_CASE("effects: loops keep running and keep moving until killed")
{
    for (const EffectPreset &p : EffectPresets::All())
    {
        if (p.Category != "Loop")
            continue;
        Scene scene("Effect");
        Entity e = MakeSubject(scene);
        const uint32_t id = EffectPresets::Play(scene.GetTweens(), e.GetHandle(), p);
        float peak = 0.0f;
        Advance(scene, p.Duration * 3.3f, &peak, e);
        CHECK(scene.GetTweens().IsRunning(id));
        if (peak <= 2.0f)
            SIMPTest::Fail(__FILE__, __LINE__, p.Name + " barely moves (peak " + std::to_string(peak) + ")");
        scene.GetTweens().Kill(id);
        scene.GetTweens().Update(1.0f / 60.0f);
        CHECK(!scene.GetTweens().IsRunning(id));
    }
}

TEST_CASE("effects: options change duration, distance, delay, amount and looping")
{
    {
        Scene scene("Effect");
        Entity e = MakeSubject(scene);
        EffectOptions options;
        options.Distance = 2.0f;
        EffectPresets::Play(scene.GetTweens(), e.GetHandle(), *EffectPresets::Find("slideInLeft"), options);
        scene.GetTweens().Update(0.001f);
        CHECK(Read(e).x < 50.0f - 390.0f);
    }
    {
        Scene scene("Effect");
        Entity e = MakeSubject(scene);
        EffectOptions options;
        options.Duration = 2.0f;
        const uint32_t id = EffectPresets::Play(scene.GetTweens(), e.GetHandle(), *EffectPresets::Find("popIn"), options);
        Advance(scene, 1.0f);
        CHECK(scene.GetTweens().IsRunning(id));
        Advance(scene, 1.2f);
        CHECK(!scene.GetTweens().IsRunning(id));
        CHECK(AtRest(Read(e)));
    }
    {
        Scene scene("Effect");
        Entity e = MakeSubject(scene);
        EffectOptions options;
        options.Delay = 1.0f;
        EffectPresets::Play(scene.GetTweens(), e.GetHandle(), *EffectPresets::Find("fadeIn"), options);
        Advance(scene, 0.5f);
        CHECK(Read(e).alpha == 0.0f);
        Advance(scene, 1.0f);
        CHECK(Read(e).alpha > 200.0f);
        Advance(scene, 0.6f);
        CHECK(Read(e).alpha == 255.0f);
    }
    {
        Scene scene("Effect");
        Entity e = MakeSubject(scene);
        EffectOptions small, big;
        small.Amount = 0.5f;
        big.Amount = 2.0f;
        const EffectPreset &pulse = *EffectPresets::Find("pulse");
        EffectPresets::Play(scene.GetTweens(), e.GetHandle(), pulse, small);
        float smallPeak = 0.0f;
        Advance(scene, pulse.Duration + 0.1f, &smallPeak, e);
        EffectPresets::Play(scene.GetTweens(), e.GetHandle(), pulse, big);
        float bigPeak = 0.0f;
        Advance(scene, pulse.Duration + 0.1f, &bigPeak, e);
        CHECK(bigPeak > smallPeak * 3.0f);
        CHECK(AtRest(Read(e)));
    }
    {
        Scene scene("Effect");
        Entity e = MakeSubject(scene);
        EffectOptions options;
        options.Loop = 2;
        const EffectPreset &shake = *EffectPresets::Find("shake");
        const uint32_t id = EffectPresets::Play(scene.GetTweens(), e.GetHandle(), shake, options);
        Advance(scene, shake.Duration * 2.0f);
        CHECK(scene.GetTweens().IsRunning(id));
        Advance(scene, shake.Duration * 1.2f);
        CHECK(!scene.GetTweens().IsRunning(id));
        CHECK(AtRest(Read(e)));
    }
}

TEST_CASE("effects: generated code names the effect and expands into Tween calls")
{
    const EffectPreset &pop = *EffectPresets::Find("popIn");
    CHECK(EffectPresets::OneLiner(pop) == "Effect.PopIn(self.entity)");
    CHECK(EffectPresets::PlayCall(pop) == "Effect.Play(self.entity, \"popIn\")");
    EffectOptions options;
    options.Duration = 1.0f;
    options.Delay = 0.5f;
    options.Loop = -1;
    const std::string call = EffectPresets::PlayCall(pop, options);
    CHECK(call.find("time = 1") != std::string::npos);
    CHECK(call.find("delay = 0.5") != std::string::npos);
    CHECK(call.find("loop = true") != std::string::npos);

    const std::string expanded = EffectPresets::ExpandedLua(pop);
    CHECK(expanded.find("Tween.Parallel") != std::string::npos);
    CHECK(expanded.find("Tween.From(e, {scaleX = 0, scaleY = 0}, 0.5, \"easeOutBack\")") != std::string::npos);

    const std::string shake = EffectPresets::ExpandedLua(*EffectPresets::Find("shake"));
    CHECK(shake.find("Tween.To(e, {x = \"+=14\"}, 0.5, Curves.Sine(8))") != std::string::npos);

    const std::string slide = EffectPresets::ExpandedLua(*EffectPresets::Find("slideInLeft"));
    CHECK(slide.find("x = \"-=200\", y = \"+=0\"") != std::string::npos);

    const std::string spin = EffectPresets::ExpandedLua(*EffectPresets::Find("spin"));
    CHECK(spin.find("loop = true") != std::string::npos);

    for (const EffectPreset &p : EffectPresets::All())
    {
        const std::string code = EffectPresets::ExpandedLua(p);
        CHECK(code.rfind("local e = self.entity\nTween.", 0) == 0);
        CHECK(std::count(code.begin(), code.end(), '(') == std::count(code.begin(), code.end(), ')'));
        CHECK(std::count(code.begin(), code.end(), '{') == std::count(code.begin(), code.end(), '}'));
    }
}

TEST_CASE("effects: Lua plays presets through one-liners and Effect.Play")
{
    namespace fs = std::filesystem;
    const fs::path root = fs::temp_directory_path() / "simp_effect_tests";
    fs::create_directories(root / "scripts");
    {
        std::ofstream file(root / "scripts" / "effects.lua");
        file << R"(
local S = {}
function S:OnCreate()
    Game.Set("fx_names", #Effect.Names())
    Game.Set("fx_entrance", #Effect.Names("Entrance"))
    Game.Set("fx_categories", #Effect.Categories())
    local info = Effect.Info("popIn")
    Game.Set("fx_info", (info and info.label == "Pop In" and info.category == "Entrance" and info.duration == 0.5) and 1 or 0)
    Game.Set("fx_unknown_info", Effect.Info("nope") == nil and 1 or 0)
    Game.Set("fx_bad_name", pcall(Effect.Play, self.entity, "nope") and 0 or 1)
    Game.Set("fx_bad_member", pcall(function() return Effect.NotAnEffect end) and 0 or 1)
    self.handle = Effect.PopIn(self.entity, {time = 0.3})
    Game.Set("fx_handle", self.handle ~= nil and 1 or 0)
end
return S
)";
    }
    VFS::Mount("assets", root.string());
    {
        Scene scene("Lua");
        Entity e = MakeSubject(scene);
        e.AddComponent<ScriptComponent>().BindLua("assets://scripts/effects.lua");
        scene.OnUpdate(1.0f / 60.0f);
        CHECK(Read(e).sx < 0.5f);
        for (int i = 0; i < 40; ++i)
            scene.OnUpdate(1.0f / 60.0f);
        CHECK(AtRest(Read(e)));
    }
    VFS::UnMount("assets");
    fs::remove_all(root);
    CHECK(GameVariables::Get("fx_names") >= 36.0);
    CHECK(GameVariables::Get("fx_entrance") == 12.0);
    CHECK(GameVariables::Get("fx_categories") == 4.0);
    CHECK(GameVariables::Get("fx_info") == 1.0);
    CHECK(GameVariables::Get("fx_unknown_info") == 1.0);
    CHECK(GameVariables::Get("fx_bad_name") == 1.0);
    CHECK(GameVariables::Get("fx_bad_member") == 1.0);
    CHECK(GameVariables::Get("fx_handle") == 1.0);
}
