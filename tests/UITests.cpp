#include "TestFramework.h"

#include <Engine/Input/Input.h>
#include <Engine/Input/InputActions.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Systems/UISystem.h>

#include <SDL3/SDL.h>

#include <cmath>

using namespace SIMPEngine;

namespace
{
    Entity MakeCanvas(Scene &scene, float w = 1280.0f, float h = 720.0f, int scaleMode = 1)
    {
        Entity canvas = scene.BuildEntity("Canvas").Build();
        auto &c = canvas.AddComponent<CanvasComponent>();
        c.referenceSize = {w, h};
        c.scaleMode = scaleMode;
        return canvas;
    }

    Entity MakeNode(Scene &scene, Entity parent, const char *name, glm::vec2 size)
    {
        Entity node = scene.BuildEntity(name).Build();
        auto &rect = node.AddComponent<UIRectComponent>();
        rect.size = size;
        scene.SetParent(node.GetHandle(), parent.GetHandle());
        return node;
    }

    glm::vec4 RectOf(Entity entity)
    {
        return entity.GetComponent<UIRectComponent>().rect;
    }
}

TEST_CASE("Canvas scales with the screen and anchors place nodes")
{
    Scene scene("ui");
    Entity canvas = MakeCanvas(scene);
    Entity topLeft = MakeNode(scene, canvas, "TopLeft", {100.0f, 40.0f});
    auto &tl = topLeft.GetComponent<UIRectComponent>();
    tl.anchorMin = tl.anchorMax = {0.0f, 1.0f};
    tl.pivot = {0.0f, 1.0f};
    tl.position = {10.0f, -10.0f};

    Entity center = MakeNode(scene, canvas, "Center", {200.0f, 100.0f});

    Entity stretch = MakeNode(scene, canvas, "Stretch", {0.0f, 0.0f});
    auto &st = stretch.GetComponent<UIRectComponent>();
    st.anchorMin = {0.0f, 0.0f};
    st.anchorMax = {1.0f, 0.0f};
    st.margin = {20.0f, 5.0f, 30.0f, 0.0f};
    st.size.y = 50.0f;
    st.pivot = {0.5f, 0.0f};
    st.position = {0.0f, 5.0f};

    UISystem::Layout(scene.GetRegistry(), {1280.0f, 720.0f});
    CHECK_NEAR(canvas.GetComponent<CanvasComponent>().scale, 1.0f, 1e-4);
    glm::vec4 r = RectOf(topLeft);
    CHECK_NEAR(r.x, 10.0f, 1e-3);
    CHECK_NEAR(r.y, 720.0f - 10.0f - 40.0f, 1e-3);
    CHECK_NEAR(r.z, 100.0f, 1e-3);
    r = RectOf(center);
    CHECK_NEAR(r.x, 540.0f, 1e-3);
    CHECK_NEAR(r.y, 310.0f, 1e-3);
    r = RectOf(stretch);
    CHECK_NEAR(r.x, 20.0f, 1e-3);
    CHECK_NEAR(r.z, 1280.0f - 50.0f, 1e-3);
    CHECK_NEAR(r.w, 50.0f, 1e-3);

    UISystem::Layout(scene.GetRegistry(), {2560.0f, 1440.0f});
    CHECK_NEAR(canvas.GetComponent<CanvasComponent>().scale, 2.0f, 1e-3);
    CHECK_NEAR(canvas.GetComponent<CanvasComponent>().size.x, 1280.0f, 1e-2);
    r = RectOf(center);
    CHECK_NEAR(r.x, 540.0f, 1e-2);

    canvas.GetComponent<CanvasComponent>().scaleMode = 0;
    UISystem::Layout(scene.GetRegistry(), {2560.0f, 1440.0f});
    CHECK_NEAR(canvas.GetComponent<CanvasComponent>().size.x, 2560.0f, 1e-3);
}

TEST_CASE("Row and column layouts arrange children")
{
    Scene scene("ui");
    Entity canvas = MakeCanvas(scene, 800.0f, 600.0f, 0);
    Entity column = MakeNode(scene, canvas, "Column", {200.0f, 400.0f});
    auto &layout = column.AddComponent<UILayoutComponent>();
    layout.direction = 1;
    layout.spacing = 10.0f;
    layout.padding = {0.0f, 0.0f, 0.0f, 0.0f};
    layout.align = 1;
    Entity a = MakeNode(scene, column, "A", {100.0f, 50.0f});
    Entity b = MakeNode(scene, column, "B", {60.0f, 30.0f});

    UISystem::Layout(scene.GetRegistry(), {800.0f, 600.0f});
    const glm::vec4 col = RectOf(column);
    glm::vec4 ra = RectOf(a), rb = RectOf(b);
    CHECK_NEAR(ra.y + ra.w, col.y + col.w, 1e-3);
    CHECK_NEAR(rb.y + rb.w, ra.y - 10.0f, 1e-3);
    CHECK_NEAR(ra.x, col.x + (200.0f - 100.0f) * 0.5f, 1e-3);
    CHECK_NEAR(rb.x, col.x + (200.0f - 60.0f) * 0.5f, 1e-3);

    layout.direction = 0;
    layout.justify = 1;
    layout.align = 2;
    UISystem::Layout(scene.GetRegistry(), {800.0f, 600.0f});
    ra = RectOf(a);
    rb = RectOf(b);
    CHECK_NEAR(rb.x, ra.x + 100.0f + 10.0f, 1e-3);
    const float total = 100.0f + 10.0f + 60.0f;
    CHECK_NEAR(ra.x, col.x + (200.0f - total) * 0.5f, 1e-3);
    CHECK_NEAR(ra.y, col.y, 1e-3);
}

TEST_CASE("UI buttons click and consume the mouse")
{
    InputActions::InstallBuiltinDefaults();
    Input::ResetAllKeys();
    Input::SetEnabled(true);
    Input::SetViewport(0.0f, 0.0f, 1280.0f, 720.0f);

    Scene scene("ui");
    Entity canvas = MakeCanvas(scene);
    Entity button = MakeNode(scene, canvas, "Play", {200.0f, 60.0f});
    button.AddComponent<UIPanelComponent>();
    button.AddComponent<UIButtonComponent>();
    Entity label = MakeNode(scene, button, "Text", {200.0f, 60.0f});
    label.AddComponent<UILabelComponent>();

    Input::BeginFrame();
    Input::OnMouseMoved(10.0f, 10.0f);
    scene.OnUpdate(0.016f);
    CHECK(!scene.IsMouseOverUI());
    CHECK(!button.GetComponent<UIButtonComponent>().hovering);

    Input::BeginFrame();
    Input::OnMouseMoved(640.0f, 360.0f);
    scene.OnUpdate(0.016f);
    CHECK(scene.IsMouseOverUI());
    CHECK(button.GetComponent<UIButtonComponent>().hovering);

    Input::BeginFrame();
    Input::OnMouseButtonPressed(1);
    scene.OnUpdate(0.016f);
    CHECK(button.GetComponent<UIButtonComponent>().pressed);

    Input::BeginFrame();
    Input::OnMouseButtonReleased(1);
    scene.OnUpdate(0.016f);
    CHECK(!button.GetComponent<UIButtonComponent>().pressed);

    Input::BeginFrame();
    Input::OnMouseMoved(10.0f, 10.0f);
    scene.OnUpdate(0.016f);
    CHECK(!button.GetComponent<UIButtonComponent>().hovering);
    Input::ClearViewport();
    Input::ResetAllKeys();
}

TEST_CASE("UI keyboard and gamepad navigation moves focus")
{
    InputActions::InstallBuiltinDefaults();
    Input::ResetAllKeys();
    Input::SetEnabled(true);
    Input::SetViewport(0.0f, 0.0f, 1280.0f, 720.0f);
    Input::OnMouseMoved(5.0f, 5.0f);

    Scene scene("ui");
    Entity canvas = MakeCanvas(scene);
    Entity column = MakeNode(scene, canvas, "Menu", {300.0f, 300.0f});
    auto &layout = column.AddComponent<UILayoutComponent>();
    layout.align = 1;
    Entity first = MakeNode(scene, column, "First", {200.0f, 50.0f});
    first.AddComponent<UIButtonComponent>();
    Entity second = MakeNode(scene, column, "Second", {200.0f, 50.0f});
    second.AddComponent<UIButtonComponent>();

    Input::BeginFrame();
    InputActions::Update();
    scene.OnUpdate(0.016f);

    Input::BeginFrame();
    Input::OnKeyPressed(SDLK_S);
    InputActions::Update();
    scene.OnUpdate(0.016f);
    CHECK(!scene.GetUI().IsFocusVisible());
    Input::OnKeyReleased(SDLK_S);

    Input::BeginFrame();
    InputActions::Update();
    scene.OnUpdate(0.016f);

    Input::BeginFrame();
    Input::OnKeyPressed(SDLK_TAB);
    InputActions::Update();
    scene.OnUpdate(0.016f);
    CHECK(scene.GetUI().IsFocusVisible());
    CHECK(scene.GetUI().GetFocused() == first.GetHandle());
    Input::OnKeyReleased(SDLK_TAB);

    Input::BeginFrame();
    InputActions::Update();
    scene.OnUpdate(0.016f);

    Input::BeginFrame();
    Input::OnKeyPressed(SDLK_S);
    InputActions::Update();
    scene.OnUpdate(0.016f);
    CHECK(scene.GetUI().GetFocused() == second.GetHandle());
    CHECK(second.GetComponent<UIButtonComponent>().focused);
    Input::OnKeyReleased(SDLK_S);
    Input::ClearViewport();
    Input::ResetAllKeys();
}
