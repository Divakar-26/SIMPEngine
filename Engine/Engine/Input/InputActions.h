#pragma once

#include <optional>
#include <string>
#include <vector>

namespace SIMPEngine
{
    enum class BindingDevice
    {
        Key,
        Mouse,
        PadButton,
        PadAxis
    };

    struct InputBinding
    {
        BindingDevice Device = BindingDevice::Key;
        int Code = 0;
        float Scale = 1.0f;

        bool operator==(const InputBinding &other) const
        {
            return Device == other.Device && Code == other.Code && Scale == other.Scale;
        }
    };

    struct InputAction
    {
        std::string Name;
        std::vector<InputBinding> Bindings;

        float Value = 0.0f;
        float Injected = 0.0f;
        bool Down = false;
        bool JustPressed = false;
        bool JustReleased = false;
    };

    class InputActions
    {
    public:
        static constexpr float PressThreshold = 0.5f;

        static void InstallBuiltinDefaults();
        static void Update();

        static bool Has(const std::string &name);
        static bool IsDown(const std::string &name);
        static bool IsJustPressed(const std::string &name);
        static bool IsJustReleased(const std::string &name);
        static float GetAxis(const std::string &name);
        static void GetVector(const std::string &name, float &x, float &y);
        static void Inject(const std::string &name, float value);

        static const std::vector<InputAction> &GetActions();
        static void SetDefaults(const std::vector<InputAction> &actions);
        static const std::vector<InputAction> &GetDefaults();
        static InputAction *Find(const std::string &name);
        static bool AddAction(const std::string &name);
        static bool RemoveAction(const std::string &name);

        static std::vector<std::string> GetBindingTexts(const std::string &name);
        static bool Rebind(const std::string &name, const std::string &binding);
        static bool AddBinding(const std::string &name, const std::string &binding);
        static bool RemoveBinding(const std::string &name, const std::string &binding);
        static bool ClearBindings(const std::string &name);
        static bool ResetAction(const std::string &name);
        static void ResetAll();
        static bool IsOverridden(const std::string &name);

        static std::string ToText(const InputBinding &binding);
        static std::optional<InputBinding> FromText(const std::string &text);
        static std::string DisplayText(const InputBinding &binding);
        static std::optional<std::string> PollPressedBinding();

        static bool LoadFile(const std::string &path);
        static bool SaveFile(const std::string &path);
        static void LoadProject(const std::string &projectDirectory);
        static bool SaveProject(const std::string &projectDirectory);
        static void CloseProject();
        static void EnableUserBindings(const std::string &gameName);
        static void DisableUserBindings();
        static bool SaveUserBindings();
        static bool IsProjectLoaded();

    private:
        static void Compute(InputAction &action);
        static void LoadUserBindings();
    };
}
