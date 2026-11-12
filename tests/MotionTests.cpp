#include "TestFramework.h"

#include <Engine/Core/GameVariables.h>
#include <Engine/Core/VFS.h>
#include <Engine/Motion/MotionHost.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scripting/LuaRuntime.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>

using namespace SIMPEngine;
namespace fs = std::filesystem;

namespace
{
    void Write(const fs::path &path, const std::string &text, int secondsAhead = 0)
    {
        {
            std::ofstream file(path, std::ios::trunc);
            file << text;
        }
        std::error_code ec;
        fs::last_write_time(path, fs::file_time_type::clock::now() + std::chrono::seconds(secondsAhead), ec);
    }

    const char *Script(int version)
    {
        static std::string text[3] = {
            R"(
Video.Duration = 5
local S = {}
function S:OnCreate() self.t = 0 end
function S:OnUpdate(dt)
    self.t = Video.Step(self.t, dt)
    Game.Set("motion_t", self.t)
    Game.Set("motion_version", 1)
    Game.Set("motion_driven", Video.Driven and 1 or 0)
    Game.Set("motion_recording", Video.Recording and 1 or 0)
end
return S
)",
            R"(
Video.Duration = 8
local S = {}
function S:OnUpdate(dt)
    Game.Set("motion_version", 2)
end
return S
)",
            "this is not lua ((("};
        return text[version].c_str();
    }
}

TEST_CASE("motion host: loads a script, runs it on a clock the host owns and reports its duration")
{
    const fs::path root = fs::temp_directory_path() / "simp_motion_host";
    fs::remove_all(root);
    fs::create_directories(root / "scripts");
    Write(root / "scripts" / "video.lua", Script(0));
    VFS::Mount("assets", root.string());
    {
        MotionHost host;
        CHECK(host.Load("assets://scripts/video.lua", true));
        CHECK(host.Error().empty());
        CHECK_NEAR(host.Duration(), 5.0, 1e-6);
        CHECK(GameVariables::Get("motion_driven") == 1.0);
        CHECK(GameVariables::Get("motion_recording") == 1.0);
        host.Step(1.0f);
        host.Step(0.5f);
        CHECK_NEAR(host.Time(), 1.5, 1e-5);
        CHECK_NEAR(GameVariables::Get("motion_t"), 1.5, 1e-4);
        host.Seek(3.25f);
        CHECK_NEAR(GameVariables::Get("motion_t"), 3.25, 1e-4);
        host.Seek(-2.0f);
        CHECK_NEAR(host.Time(), 0.0, 1e-6);
        CHECK(!host.PollReload());
        CHECK(host.ReloadCount() == 0);
    }
    VFS::UnMount("assets");
    fs::remove_all(root);
}

TEST_CASE("motion host: saving the script reloads it, a broken script reports its error and fixing it recovers")
{
    const fs::path root = fs::temp_directory_path() / "simp_motion_reload";
    fs::remove_all(root);
    fs::create_directories(root / "scripts");
    Write(root / "scripts" / "video.lua", Script(0));
    VFS::Mount("assets", root.string());
    {
        MotionHost host;
        CHECK(host.Load("assets://scripts/video.lua", false));
        CHECK(GameVariables::Get("motion_recording") == 0.0);
        host.Step(2.0f);
        CHECK(GameVariables::Get("motion_version") == 1.0);

        Write(root / "scripts" / "video.lua", Script(1), 5);
        CHECK(host.PollReload());
        CHECK(host.ReloadCount() == 1);
        CHECK(host.Error().empty());
        CHECK_NEAR(host.Duration(), 8.0, 1e-6);
        CHECK_NEAR(host.Time(), 2.0, 1e-5);
        host.Step(0.1f);
        CHECK(GameVariables::Get("motion_version") == 2.0);
        CHECK(!host.PollReload());

        Write(root / "scripts" / "video.lua", Script(2), 10);
        CHECK(host.PollReload());
        CHECK(!host.Error().empty());
        host.Step(0.1f);

        Write(root / "scripts" / "video.lua", Script(0), 15);
        CHECK(host.PollReload());
        CHECK(host.Error().empty());
        host.Step(0.1f);
        CHECK(GameVariables::Get("motion_version") == 1.0);

        Write(root / "scripts" / "helper.lua", "return {}", 20);
        CHECK(host.PollReload());
    }
    VFS::UnMount("assets");
    fs::remove_all(root);
}

TEST_CASE("motion host: required modules are reloaded together with the script")
{
    const fs::path root = fs::temp_directory_path() / "simp_motion_modules";
    fs::remove_all(root);
    fs::create_directories(root / "scripts");
    const std::string main = R"(
package.path = ")" + (root / "scripts").generic_string() + R"(/?.lua;" .. package.path
local helper = require "helper"
local S = {}
function S:OnUpdate(dt) Game.Set("motion_helper", helper.value) end
return S
)";
    Write(root / "scripts" / "video.lua", main);
    Write(root / "scripts" / "helper.lua", "return {value = 11}");
    VFS::Mount("assets", root.string());
    {
        MotionHost host;
        CHECK(host.Load("assets://scripts/video.lua", false));
        host.Step(0.1f);
        CHECK(GameVariables::Get("motion_helper") == 11.0);
        Write(root / "scripts" / "helper.lua", "return {value = 22}", 5);
        CHECK(host.PollReload());
        host.Step(0.1f);
        CHECK(GameVariables::Get("motion_helper") == 22.0);
    }
    VFS::UnMount("assets");
    fs::remove_all(root);
}

TEST_CASE("effects: Effect.Chain runs effects one after another and Effect.Stagger offsets several targets")
{
    const fs::path root = fs::temp_directory_path() / "simp_effect_chain";
    fs::remove_all(root);
    fs::create_directories(root / "scripts");
    Write(root / "scripts" / "chain.lua", R"(
local S = {}
function S:OnCreate()
    local chain = Effect.Chain(self.entity, {"popIn", {"shake", {time = 0.2}}, "fadeOut"})
    Game.Set("chain_ok", chain ~= nil and 1 or 0)
    local list = {Scene.Find("A"), Scene.Find("B"), Scene.Find("C")}
    local handles = Effect.Stagger(list, "fadeOut", 0.5, {time = 0.2})
    Game.Set("stagger_count", #handles)
    Game.Set("bad_chain", pcall(Effect.Chain, self.entity, {"float"}) and 0 or 1)
end
return S
)");
    VFS::Mount("assets", root.string());
    {
        Scene scene("Chain");
        std::map<std::string, Entity> named;
        for (const char *name : {"A", "B", "C", "Subject"})
        {
            Entity e = scene.CreateEntity(name);
            e.AddComponent<RenderComponent>();
            named[name] = e;
        }
        Entity subject = named["Subject"];
        subject.AddComponent<ScriptComponent>().BindLua("assets://scripts/chain.lua");
        scene.OnUpdate(1.0f / 60.0f);
        CHECK(GameVariables::Get("chain_ok") == 1.0);
        CHECK(GameVariables::Get("stagger_count") == 3.0);
        CHECK(GameVariables::Get("bad_chain") == 1.0);

        auto alpha = [&](const char *name)
        { return static_cast<int>(named[name].GetComponent<RenderComponent>().color.a); };
        auto advance = [&](int frames)
        {
            for (int i = 0; i < frames; ++i)
                scene.OnUpdate(1.0f / 60.0f);
        };
        advance(5);
        CHECK(alpha("A") < 255);
        CHECK(alpha("B") == 255);
        CHECK(alpha("C") == 255);
        advance(30);
        CHECK(alpha("A") == 0);
        CHECK(alpha("B") < 255);
        CHECK(alpha("C") == 255);
        advance(45);
        CHECK(alpha("B") == 0);
        CHECK(alpha("C") == 0);
        CHECK(alpha("Subject") == 0);
    }
    VFS::UnMount("assets");
    fs::remove_all(root);
}

TEST_CASE("lua helpers: Glyphs splits text into drawable glyph paths with bounds")
{
    VFS::Mount("engine", SIMP_TEST_ASSETS);
    CHECK(LuaRuntime::Get().ExecuteString(R"LUA(
        local function near(a, b) return math.abs(a - b) < 1e-3 end
        local glyphs = Glyphs.New("Hi!", 100)
        assert(glyphs:Count() == 3)
        local w, h = glyphs:Size()
        assert(w > 50 and h > 40)
        local x0, y0, x1, y1 = glyphs:Bounds()
        assert(x0 < x1 and y0 < y1)
        local list = Path.TextGlyphs("a b", 60, "left")
        assert(#list == 2 and list[1].index == 1 and list[2].index == 3 and list[2].codepoint == string.byte("b"))
        assert(list[1].minX < list[1].maxX and list[2].minX > list[1].maxX)
        assert(not list[1].path:IsEmpty())
        local centered = Glyphs.New("HH", 80, "center")
        local cx0, _, cx1 = centered:Bounds()
        assert(math.abs(cx0 + cx1) < 6)
        assert(select("#", Path.TextGlyphs("   ", 10)) == 2)
        assert(not pcall(Glyphs.New, "   ", 10))
        assert(type(Glyphs.Draw) == "function" and type(Backdrop.Grid) == "function" and type(Backdrop.Fill) == "function")
    )LUA", "glyphs"));
    VFS::UnMount("engine");
}
