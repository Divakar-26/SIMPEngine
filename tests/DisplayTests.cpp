#include "TestFramework.h"

#include <Engine/Core/Display.h>
#include <Engine/Math/Camera2D.h>
#include <Engine/Project/Project.h>

#include <filesystem>

using namespace SIMPEngine;

TEST_CASE("Display layouts follow the aspect and stretch modes")
{
    DisplaySettings design;
    design.Width = 1920;
    design.Height = 1080;

    design.Aspect = AspectMode::KeepHeight;
    ViewLayout layout = Display::Compute(design, 1000, 1000);
    CHECK(!layout.Offscreen);
    CHECK(layout.Width == 1000);
    CHECK(layout.RenderHeight == 1000);

    design.Aspect = AspectMode::Keep;
    layout = Display::Compute(design, 1000, 1000);
    CHECK(layout.Offscreen);
    CHECK(layout.Width == 1000);
    CHECK(layout.Height == 563);
    CHECK(layout.X == 0);
    CHECK(layout.Y == 218);

    layout = Display::Compute(design, 2400, 1080);
    CHECK(layout.Height == 1080);
    CHECK(layout.Width == 1920);
    CHECK(layout.X == 240);
    CHECK(layout.Y == 0);

    layout = Display::Compute(design, 1920, 1080);
    CHECK(!layout.Offscreen);

    design.Aspect = AspectMode::KeepHeight;
    design.Stretch = StretchMode::Fixed;
    layout = Display::Compute(design, 2400, 1000);
    CHECK(layout.Offscreen);
    CHECK(layout.RenderHeight == 1080);
    CHECK(layout.RenderWidth == 2592);

    design.Aspect = AspectMode::KeepWidth;
    layout = Display::Compute(design, 1000, 1000);
    CHECK(layout.RenderWidth == 1920);
    CHECK(layout.RenderHeight == 1920);

    design.Aspect = AspectMode::Expand;
    layout = Display::Compute(design, 1000, 1000);
    CHECK(layout.RenderWidth == 1920);
    layout = Display::Compute(design, 3000, 1000);
    CHECK(layout.RenderHeight == 1080);
}

TEST_CASE("The camera shows the game resolution")
{
    DisplaySettings settings;
    settings.Width = 1280;
    settings.Height = 720;
    settings.Aspect = AspectMode::KeepHeight;
    glm::vec2 half = Display::HalfExtents(settings, 1.0f, 2.0f);
    CHECK_NEAR(half.y, 360.0f, 1e-3);
    CHECK_NEAR(half.x, 720.0f, 1e-3);

    settings.Aspect = AspectMode::KeepWidth;
    half = Display::HalfExtents(settings, 0.5f, 2.0f);
    CHECK_NEAR(half.x, 320.0f, 1e-3);
    CHECK_NEAR(half.y, 160.0f, 1e-3);

    settings.Aspect = AspectMode::Expand;
    half = Display::HalfExtents(settings, 1.0f, 1.0f);
    CHECK_NEAR(half.x, 640.0f, 1e-3);
    CHECK_NEAR(half.y, 640.0f, 1e-3);
    half = Display::HalfExtents(settings, 1.0f, 3.0f);
    CHECK_NEAR(half.y, 360.0f, 1e-3);

    Display::Set(settings);
    Camera2D camera(1.0f);
    camera.SetViewportSize(1280.0f, 720.0f);
    const glm::mat4 projection = camera.GetProjectionMatrix();
    CHECK_NEAR(projection[1][1], 1.0f / 360.0f, 1e-6);
    Display::Reset();
    camera.SetViewportSize(1920.0f, 1080.0f);
    CHECK_NEAR(camera.GetProjectionMatrix()[1][1], 1.0f / 540.0f, 1e-6);
}

TEST_CASE("Project display and window settings round-trip")
{
    const std::filesystem::path dir = std::filesystem::temp_directory_path() / "simp_display_project";
    std::filesystem::create_directories(dir);

    ProjectSettings settings;
    settings.Name = "Shape";
    settings.ViewWidth = 1152;
    settings.ViewHeight = 648;
    settings.Aspect = 3;
    settings.Stretch = 1;
    settings.Maximized = true;
    settings.Borderless = true;
    settings.Orientation = "portrait";
    settings.WindowWidth = 900;
    CHECK(Project::Save(dir, settings));

    ProjectSettings loaded;
    CHECK(Project::Load(dir, loaded));
    CHECK(loaded.ViewWidth == 1152);
    CHECK(loaded.ViewHeight == 648);
    CHECK(loaded.Aspect == 3);
    CHECK(loaded.Stretch == 1);
    CHECK(loaded.Maximized);
    CHECK(loaded.Borderless);
    CHECK(loaded.Orientation == "portrait");
    CHECK(loaded.WindowWidth == 900);
    std::filesystem::remove_all(dir);
}
