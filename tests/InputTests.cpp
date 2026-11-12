#include "TestFramework.h"

#include <Engine/Input/Input.h>
#include <Engine/Input/InputActions.h>

#include <SDL3/SDL.h>

#include <filesystem>

using namespace SIMPEngine;

namespace
{
    void Frame()
    {
        Input::BeginFrame();
    }
}

TEST_CASE("Input actions read keys, edges and axes")
{
    InputActions::InstallBuiltinDefaults();
    Input::ResetAllKeys();
    Input::SetEnabled(true);

    Frame();
    InputActions::Update();
    CHECK(!InputActions::IsDown("Jump"));
    CHECK_NEAR(InputActions::GetAxis("MoveX"), 0.0f, 0.0001f);

    Frame();
    Input::OnKeyPressed(SDLK_SPACE);
    Input::OnKeyPressed(SDLK_A);
    InputActions::Update();
    CHECK(InputActions::IsDown("Jump"));
    CHECK(InputActions::IsJustPressed("Jump"));
    CHECK_NEAR(InputActions::GetAxis("MoveX"), -1.0f, 0.0001f);

    Frame();
    InputActions::Update();
    CHECK(InputActions::IsDown("Jump"));
    CHECK(!InputActions::IsJustPressed("Jump"));

    Frame();
    Input::OnKeyReleased(SDLK_SPACE);
    Input::OnKeyReleased(SDLK_A);
    InputActions::Update();
    CHECK(!InputActions::IsDown("Jump"));
    CHECK(InputActions::IsJustReleased("Jump"));
    Input::ResetAllKeys();
}

TEST_CASE("Gamepad axes feed actions with a deadzone")
{
    InputActions::InstallBuiltinDefaults();
    Input::ResetAllKeys();
    Input::OnGamepadConnected(true);
    CHECK(Input::IsGamepadConnected());

    Input::OnGamepadAxis(SDL_GAMEPAD_AXIS_LEFTX, 0.1f);
    InputActions::Update();
    CHECK_NEAR(InputActions::GetAxis("MoveX"), 0.0f, 0.0001f);

    Input::OnGamepadAxis(SDL_GAMEPAD_AXIS_LEFTX, 1.0f);
    InputActions::Update();
    CHECK_NEAR(InputActions::GetAxis("MoveX"), 1.0f, 0.0001f);

    Input::OnGamepadAxis(SDL_GAMEPAD_AXIS_LEFTY, 1.0f);
    InputActions::Update();
    CHECK_NEAR(InputActions::GetAxis("MoveY"), -1.0f, 0.0001f);

    float x = 0, y = 0;
    InputActions::GetVector("Move", x, y);
    CHECK_NEAR(x * x + y * y, 1.0f, 0.001f);

    Input::OnGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH, true);
    InputActions::Update();
    CHECK(InputActions::IsJustPressed("Jump"));
    Input::OnGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH, false);
    Input::OnGamepadConnected(false);
    CHECK(!Input::IsGamepadConnected());
    Input::ResetAllKeys();
}

TEST_CASE("Input bindings parse, rebind and persist")
{
    InputActions::InstallBuiltinDefaults();

    auto space = InputActions::FromText("Key.Space");
    CHECK(space.has_value());
    CHECK(space && space->Code == SDLK_SPACE);
    CHECK(InputActions::ToText(*space) == "Key.Space");
    auto negative = InputActions::FromText("-Pad.DPadLeft");
    CHECK(negative && negative->Scale < 0.0f);
    CHECK(!InputActions::FromText("Key.NotAKey").has_value());
    CHECK(!InputActions::FromText("garbage").has_value());

    CHECK(InputActions::Rebind("Jump", "Key.J"));
    bool hasJ = false, hasSpace = false, hasPad = false;
    for (const std::string &text : InputActions::GetBindingTexts("Jump"))
    {
        hasJ = hasJ || text == "Key.J";
        hasSpace = hasSpace || text == "Key.Space";
        hasPad = hasPad || text == "Pad.South";
    }
    CHECK(hasJ);
    CHECK(!hasSpace);
    CHECK(hasPad);
    CHECK(InputActions::IsOverridden("Jump"));
    CHECK(!InputActions::Rebind("Missing", "Key.J"));

    const std::string file = (std::filesystem::temp_directory_path() / "simp_input_test.yaml").string();
    CHECK(InputActions::SaveFile(file));
    InputActions::InstallBuiltinDefaults();
    CHECK(InputActions::LoadFile(file));
    CHECK(InputActions::Has("MoveX"));
    CHECK(InputActions::GetBindingTexts("MoveX").size() == 7);
    std::filesystem::remove(file);

    InputActions::InstallBuiltinDefaults();
    CHECK(InputActions::Rebind("Jump", "Key.J"));
    CHECK(InputActions::ResetAction("Jump"));
    CHECK(!InputActions::IsOverridden("Jump"));
    CHECK(InputActions::GetBindingTexts("Jump").size() == 4);
}

TEST_CASE("Injected values drive actions for touch input")
{
    InputActions::InstallBuiltinDefaults();
    Input::ResetAllKeys();
    InputActions::Inject("MoveX", 0.8f);
    InputActions::Update();
    CHECK_NEAR(InputActions::GetAxis("MoveX"), 0.8f, 0.0001f);
    CHECK(InputActions::IsDown("MoveX"));
    InputActions::Inject("MoveX", 0.0f);
    InputActions::Update();
    CHECK(!InputActions::IsDown("MoveX"));
}

#include <Engine/Input/TouchControls.h>

namespace
{
    SDL_Event Finger(SDL_EventType type, SDL_FingerID id, float x, float y)
    {
        SDL_Event event{};
        event.type = type;
        event.tfinger.fingerID = id;
        event.tfinger.x = x;
        event.tfinger.y = y;
        return event;
    }
}

TEST_CASE("Touch controls turn fingers into stick, buttons and a pointer")
{
    InputActions::InstallBuiltinDefaults();
    Input::ResetAllKeys();
    TouchControls::SetEnabled(true);
    const float w = 1920.0f, h = 1080.0f;
    glm::vec2 point{0.0f};
    bool down = false, up = false;

    const float unit = std::min(w, h);
    const glm::vec2 stick(unit * 0.24f, h - unit * 0.26f);
    SDL_Event press = Finger(SDL_EVENT_FINGER_DOWN, 1, stick.x / w, stick.y / h);
    CHECK(TouchControls::HandleEvent(press, w, h, point, down, up));
    CHECK(!down);
    SDL_Event drag = Finger(SDL_EVENT_FINGER_MOTION, 1, (stick.x + unit * 0.3f) / w, stick.y / h);
    TouchControls::HandleEvent(drag, w, h, point, down, up);
    CHECK_NEAR(TouchControls::GetStick().x, 1.0f, 1e-3);
    CHECK_NEAR(TouchControls::GetStick().y, 0.0f, 1e-3);
    TouchControls::Update();
    InputActions::Update();
    CHECK_NEAR(InputActions::GetAxis("MoveX"), 1.0f, 1e-3);
    CHECK(InputActions::IsDown("MoveX"));

    const glm::vec2 jump(w - unit * 0.2f, h - unit * 0.24f);
    SDL_Event jumpPress = Finger(SDL_EVENT_FINGER_DOWN, 2, jump.x / w, jump.y / h);
    TouchControls::HandleEvent(jumpPress, w, h, point, down, up);
    CHECK(TouchControls::IsJumpDown());
    TouchControls::Update();
    InputActions::Update();
    CHECK(InputActions::IsJustPressed("Jump"));

    SDL_Event world = Finger(SDL_EVENT_FINGER_DOWN, 3, 0.5f, 0.3f);
    TouchControls::HandleEvent(world, w, h, point, down, up);
    CHECK(down);
    CHECK_NEAR(point.x, 960.0f, 1e-2);
    CHECK_NEAR(point.y, 324.0f, 1e-2);
    SDL_Event second = Finger(SDL_EVENT_FINGER_DOWN, 4, 0.6f, 0.3f);
    TouchControls::HandleEvent(second, w, h, point, down, up);
    CHECK(!down);
    SDL_Event lift = Finger(SDL_EVENT_FINGER_UP, 3, 0.5f, 0.3f);
    TouchControls::HandleEvent(lift, w, h, point, down, up);
    CHECK(up);

    SDL_Event releaseStick = Finger(SDL_EVENT_FINGER_UP, 1, 0.5f, 0.5f);
    TouchControls::HandleEvent(releaseStick, w, h, point, down, up);
    CHECK_NEAR(TouchControls::GetStick().x, 0.0f, 1e-3);
    TouchControls::SetEnabled(false);
    InputActions::Update();
    CHECK(!InputActions::IsDown("MoveX"));
    Input::ResetAllKeys();
}
