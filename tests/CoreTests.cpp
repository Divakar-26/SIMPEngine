#include "TestFramework.h"

#include <Engine/Animation/Curves.h>
#include <Engine/Core/FileSystem.h>
#include <Engine/Core/VFS.h>
#include <Engine/Layer/LayerStack.h>
#include <Engine/Math/Camera2D.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/TilemapUtils.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace SIMPEngine;

TEST_CASE("vfs: aliases resolve to their mount and plain paths pass through")
{
    const std::string root = std::filesystem::temp_directory_path().string();
    VFS::Mount("testroot", root);
    auto resolved = VFS::Resolve("testroot://a/b.txt");
    CHECK(resolved.has_value());
    if (resolved)
        CHECK(std::filesystem::path(*resolved) == std::filesystem::path(root) / "a" / "b.txt");
    CHECK(!VFS::Resolve("nothere://x").has_value());
    CHECK(VFS::Resolve("relative/file.png").value_or("") == "relative/file.png");
    CHECK(VFS::UnMount("testroot"));
    CHECK(!VFS::Resolve("testroot://a").has_value());
}

TEST_CASE("camera: screen to world and back is the identity")
{
    Camera2D camera(1.5f, {120.0f, -40.0f});
    camera.SetViewportSize(800.0f, 600.0f);
    camera.SetRotation(15.0f);
    for (const glm::vec2 pixel : {glm::vec2(0.0f, 0.0f), glm::vec2(400.0f, 300.0f), glm::vec2(799.0f, 13.0f)})
    {
        const glm::vec2 world = camera.ScreenToWorld(pixel);
        const glm::vec2 back = camera.WorldToScreen(world);
        CHECK_NEAR(back.x, pixel.x, 1e-2);
        CHECK_NEAR(back.y, pixel.y, 1e-2);
    }
    const glm::vec2 center = camera.ScreenToWorld({400.0f, 300.0f});
    CHECK_NEAR(center.x, 120.0f, 1e-2);
    CHECK_NEAR(center.y, -40.0f, 1e-2);
}

namespace
{
    struct RecordingLayer : Layer
    {
        RecordingLayer(const char *name, std::vector<std::string> &log) : Layer(name), Log(log), Name(name) {}
        void OnAttach() override { Log.push_back(std::string("attach ") + Name); }
        void OnDetach() override { Log.push_back(std::string("detach ") + Name); }
        std::vector<std::string> &Log;
        const char *Name;
    };
}

TEST_CASE("layer stack: layers before overlays, detached in reverse")
{
    std::vector<std::string> log;
    {
        LayerStack stack;
        stack.PushOverlay(std::make_unique<RecordingLayer>("overlay", log));
        stack.PushLayer(std::make_unique<RecordingLayer>("a", log));
        stack.PushLayer(std::make_unique<RecordingLayer>("b", log));
        std::vector<std::string> order;
        for (auto &layer : stack)
            order.push_back(static_cast<RecordingLayer *>(layer.get())->Name);
        CHECK((order == std::vector<std::string>{"a", "b", "overlay"}));
        log.clear();
    }
    CHECK((log == std::vector<std::string>{"detach overlay", "detach b", "detach a"}));
}

TEST_CASE("tilemap: set, get, erase, flood fill and world mapping")
{
    TilemapComponent tilemap;
    tilemap.tileSize = 32.0f;
    SetTile(tilemap, 0, 0, 3);
    SetTile(tilemap, -17, 40, 7, TileFlipX);
    CHECK(GetTileId(tilemap, 0, 0) == 3);
    CHECK(GetTileId(tilemap, -17, 40) == 7);
    CHECK(GetTileId(tilemap, 1, 1) == -1);
    CHECK(CountTiles(tilemap) == 2);

    glm::ivec2 min, max;
    CHECK(GetUsedBounds(tilemap, min, max));
    CHECK(min == glm::ivec2(-17, 0));
    CHECK(max == glm::ivec2(0, 40));

    EraseTile(tilemap, -17, 40);
    CHECK(GetTileId(tilemap, -17, 40) == -1);
    CHECK(CountTiles(tilemap) == 1);

    TilemapComponent fill;
    const int filled = FloodFill(fill, {0, 0}, 1, 0, {0, 0}, {3, 2});
    CHECK(filled == 12);

    TransformComponent transform;
    transform.position = {100.0f, 50.0f};
    transform.worldTransform = transform.GetLocalTransform();
    const glm::ivec2 cell = WorldToTile(tilemap, transform, {100.0f + 32.0f * 2.5f, 50.0f + 32.0f * 1.5f});
    CHECK(cell == glm::ivec2(2, 1));
    const glm::ivec2 back = WorldToTile(tilemap, transform, TileToWorld(tilemap, transform, {5, -4}));
    CHECK(back == glm::ivec2(5, -4));
}

TEST_CASE("curves: every named curve starts at 0 and ends at 1")
{
    for (const std::string &name : Curve::Names())
    {
        Curve curve;
        CHECK(Curve::FromName(name, curve));
        const float start = curve.Evaluate(0.0f);
        const float end = curve.Evaluate(1.0f);
        if (std::abs(start) > 1e-3f || std::abs(end - 1.0f) > 1e-3f)
            SIMPTest::Fail(__FILE__, __LINE__, name + " goes from " + std::to_string(start) + " to " + std::to_string(end));
    }
    Curve unknown;
    CHECK(!Curve::FromName("definitelyNotACurve", unknown));
    CHECK_NEAR(Curve::Linear().Evaluate(0.25f), 0.25f, 1e-5);
    CHECK_NEAR(Curve::Steps(4).Evaluate(0.3f), 0.25f, 1e-5);
}

TEST_CASE("filesystem: join and normalise")
{
    const std::string joined = FileSystem::Join("a/b", "c.txt");
    CHECK(std::filesystem::path(joined) == std::filesystem::path("a/b/c.txt"));
    const std::string normal = FileSystem::Normalise("a/b/../c/./d.txt");
    CHECK(std::filesystem::path(normal) == std::filesystem::path("a/c/d.txt"));
}
