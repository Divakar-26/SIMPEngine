#include <Engine/Input/InputActions.h>
#include <Engine/Input/Input.h>
#include <Engine/Core/FileSystem.h>
#include <Engine/Core/Log.h>

#include <SDL3/SDL.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <set>

namespace SIMPEngine
{
    namespace
    {
        std::vector<InputAction> s_Actions;
        std::vector<InputAction> s_Defaults;
        std::set<std::string> s_Overridden;
        std::string s_UserFile;
        bool s_ProjectLoaded = false;

        struct NameEntry
        {
            const char *Name;
            int Code;
        };

        const NameEntry PadButtons[] = {
            {"South", SDL_GAMEPAD_BUTTON_SOUTH},
            {"East", SDL_GAMEPAD_BUTTON_EAST},
            {"West", SDL_GAMEPAD_BUTTON_WEST},
            {"North", SDL_GAMEPAD_BUTTON_NORTH},
            {"Back", SDL_GAMEPAD_BUTTON_BACK},
            {"Guide", SDL_GAMEPAD_BUTTON_GUIDE},
            {"Start", SDL_GAMEPAD_BUTTON_START},
            {"LeftStick", SDL_GAMEPAD_BUTTON_LEFT_STICK},
            {"RightStick", SDL_GAMEPAD_BUTTON_RIGHT_STICK},
            {"LeftShoulder", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
            {"RightShoulder", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
            {"DPadUp", SDL_GAMEPAD_BUTTON_DPAD_UP},
            {"DPadDown", SDL_GAMEPAD_BUTTON_DPAD_DOWN},
            {"DPadLeft", SDL_GAMEPAD_BUTTON_DPAD_LEFT},
            {"DPadRight", SDL_GAMEPAD_BUTTON_DPAD_RIGHT}};

        const NameEntry PadAxes[] = {
            {"LeftX", SDL_GAMEPAD_AXIS_LEFTX},
            {"LeftY", SDL_GAMEPAD_AXIS_LEFTY},
            {"RightX", SDL_GAMEPAD_AXIS_RIGHTX},
            {"RightY", SDL_GAMEPAD_AXIS_RIGHTY},
            {"LeftTrigger", SDL_GAMEPAD_AXIS_LEFT_TRIGGER},
            {"RightTrigger", SDL_GAMEPAD_AXIS_RIGHT_TRIGGER}};

        const NameEntry MouseButtons[] = {
            {"Left", SDL_BUTTON_LEFT},
            {"Middle", SDL_BUTTON_MIDDLE},
            {"Right", SDL_BUTTON_RIGHT},
            {"X1", SDL_BUTTON_X1},
            {"X2", SDL_BUTTON_X2}};

        template <size_t N>
        const char *NameOf(const NameEntry (&table)[N], int code)
        {
            for (const NameEntry &entry : table)
                if (entry.Code == code)
                    return entry.Name;
            return nullptr;
        }

        template <size_t N>
        std::optional<int> CodeOf(const NameEntry (&table)[N], const std::string &name)
        {
            for (const NameEntry &entry : table)
            {
                if (name.size() != std::strlen(entry.Name))
                    continue;
                bool same = true;
                for (size_t i = 0; i < name.size(); ++i)
                    if (std::tolower(static_cast<unsigned char>(name[i])) != std::tolower(static_cast<unsigned char>(entry.Name[i])))
                        same = false;
                if (same)
                    return entry.Code;
            }
            return std::nullopt;
        }

        bool SameGroup(BindingDevice a, BindingDevice b)
        {
            const bool padA = a == BindingDevice::PadButton || a == BindingDevice::PadAxis;
            const bool padB = b == BindingDevice::PadButton || b == BindingDevice::PadAxis;
            return padA == padB;
        }

        InputBinding Key(int key, float scale = 1.0f)
        {
            return {BindingDevice::Key, key, scale};
        }

        InputBinding Pad(int button, float scale = 1.0f)
        {
            return {BindingDevice::PadButton, button, scale};
        }

        InputBinding Axis(int axis, float scale = 1.0f)
        {
            return {BindingDevice::PadAxis, axis, scale};
        }

        std::string ProjectFile(const std::string &directory)
        {
            return FileSystem::Join(directory, "input.yaml");
        }

        void EmitActions(YAML::Emitter &out, const std::vector<InputAction> &actions)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "Version" << YAML::Value << 1;
            out << YAML::Key << "Actions" << YAML::Value << YAML::BeginMap;
            for (const InputAction &action : actions)
            {
                out << YAML::Key << action.Name << YAML::Value << YAML::Flow << YAML::BeginSeq;
                for (const InputBinding &binding : action.Bindings)
                    out << InputActions::ToText(binding);
                out << YAML::EndSeq;
            }
            out << YAML::EndMap << YAML::EndMap;
        }

        bool ParseActions(const std::string &text, std::vector<InputAction> &result)
        {
            YAML::Node data;
            try
            {
                data = YAML::Load(text);
            }
            catch (const YAML::Exception &e)
            {
                CORE_ERROR("Could not parse input actions: {}", e.what());
                return false;
            }
            if (!data.IsMap() || !data["Actions"] || !data["Actions"].IsMap())
                return false;

            result.clear();
            for (auto it = data["Actions"].begin(); it != data["Actions"].end(); ++it)
            {
                InputAction action;
                action.Name = it->first.as<std::string>("");
                if (action.Name.empty() || !it->second.IsSequence())
                    continue;
                const YAML::Node list = it->second;
                for (const YAML::Node &item : list)
                {
                    auto binding = InputActions::FromText(item.as<std::string>(""));
                    if (binding)
                        action.Bindings.push_back(*binding);
                    else
                        CORE_WARN("Input action '{}': unknown binding '{}'", action.Name, item.as<std::string>(""));
                }
                result.push_back(std::move(action));
            }
            return true;
        }
    }

    void InputActions::InstallBuiltinDefaults()
    {
        std::vector<InputAction> defaults;

        InputAction moveX;
        moveX.Name = "MoveX";
        moveX.Bindings = {Key(SDLK_D), Key(SDLK_RIGHT), Key(SDLK_A, -1.0f), Key(SDLK_LEFT, -1.0f),
                          Axis(SDL_GAMEPAD_AXIS_LEFTX), Pad(SDL_GAMEPAD_BUTTON_DPAD_RIGHT), Pad(SDL_GAMEPAD_BUTTON_DPAD_LEFT, -1.0f)};
        defaults.push_back(moveX);

        InputAction moveY;
        moveY.Name = "MoveY";
        moveY.Bindings = {Key(SDLK_W), Key(SDLK_UP), Key(SDLK_S, -1.0f), Key(SDLK_DOWN, -1.0f),
                          Axis(SDL_GAMEPAD_AXIS_LEFTY, -1.0f), Pad(SDL_GAMEPAD_BUTTON_DPAD_UP), Pad(SDL_GAMEPAD_BUTTON_DPAD_DOWN, -1.0f)};
        defaults.push_back(moveY);

        InputAction jump;
        jump.Name = "Jump";
        jump.Bindings = {Key(SDLK_SPACE), Key(SDLK_W), Key(SDLK_UP), Pad(SDL_GAMEPAD_BUTTON_SOUTH)};
        defaults.push_back(jump);

        InputAction interact;
        interact.Name = "Interact";
        interact.Bindings = {Key(SDLK_E), Key(SDLK_RETURN), Pad(SDL_GAMEPAD_BUTTON_WEST)};
        defaults.push_back(interact);

        InputAction pause;
        pause.Name = "Pause";
        pause.Bindings = {Key(SDLK_ESCAPE), Pad(SDL_GAMEPAD_BUTTON_START)};
        defaults.push_back(pause);

        InputAction focus;
        focus.Name = "UIFocus";
        focus.Bindings = {Key(SDLK_TAB), Pad(SDL_GAMEPAD_BUTTON_BACK)};
        defaults.push_back(focus);

        InputAction confirm;
        confirm.Name = "UIConfirm";
        confirm.Bindings = {Key(SDLK_RETURN), Key(SDLK_KP_ENTER), Pad(SDL_GAMEPAD_BUTTON_SOUTH)};
        defaults.push_back(confirm);

        InputAction cancel;
        cancel.Name = "UICancel";
        cancel.Bindings = {Key(SDLK_ESCAPE), Pad(SDL_GAMEPAD_BUTTON_EAST)};
        defaults.push_back(cancel);

        s_Defaults = defaults;
        s_Actions = defaults;
        s_Overridden.clear();
        s_ProjectLoaded = false;
    }

    void InputActions::Compute(InputAction &action)
    {
        float digital = 0.0f;
        float analog = action.Injected;
        for (const InputBinding &binding : action.Bindings)
        {
            switch (binding.Device)
            {
            case BindingDevice::Key:
                if (Input::IsPhysicalKeyPressed(binding.Code))
                    digital += binding.Scale;
                break;
            case BindingDevice::Mouse:
                if (Input::IsMouseButtonPressed(binding.Code))
                    digital += binding.Scale;
                break;
            case BindingDevice::PadButton:
                if (Input::IsGamepadButtonPressed(binding.Code))
                    digital += binding.Scale;
                break;
            case BindingDevice::PadAxis:
            {
                const float value = Input::GetGamepadAxis(binding.Code) * binding.Scale;
                if (std::abs(value) > std::abs(analog))
                    analog = value;
                break;
            }
            }
        }
        digital = std::clamp(digital, -1.0f, 1.0f);
        action.Value = std::abs(analog) > std::abs(digital) ? std::clamp(analog, -1.0f, 1.0f) : digital;
    }

    void InputActions::Update()
    {
        if (s_Actions.empty() && s_Defaults.empty())
            InstallBuiltinDefaults();
        for (InputAction &action : s_Actions)
        {
            Compute(action);
            const bool down = std::abs(action.Value) >= PressThreshold;
            action.JustPressed = down && !action.Down;
            action.JustReleased = !down && action.Down;
            action.Down = down;
        }
    }

    InputAction *InputActions::Find(const std::string &name)
    {
        for (InputAction &action : s_Actions)
            if (action.Name == name)
                return &action;
        return nullptr;
    }

    bool InputActions::Has(const std::string &name)
    {
        return Find(name) != nullptr;
    }

    bool InputActions::IsDown(const std::string &name)
    {
        const InputAction *action = Find(name);
        return action && action->Down;
    }

    bool InputActions::IsJustPressed(const std::string &name)
    {
        const InputAction *action = Find(name);
        return action && action->JustPressed;
    }

    bool InputActions::IsJustReleased(const std::string &name)
    {
        const InputAction *action = Find(name);
        return action && action->JustReleased;
    }

    float InputActions::GetAxis(const std::string &name)
    {
        const InputAction *action = Find(name);
        return action ? action->Value : 0.0f;
    }

    void InputActions::GetVector(const std::string &name, float &x, float &y)
    {
        x = GetAxis(name + "X");
        y = GetAxis(name + "Y");
        const float length = std::sqrt(x * x + y * y);
        if (length > 1.0f)
        {
            x /= length;
            y /= length;
        }
    }

    void InputActions::Inject(const std::string &name, float value)
    {
        if (InputAction *action = Find(name))
            action->Injected = std::clamp(value, -1.0f, 1.0f);
    }

    const std::vector<InputAction> &InputActions::GetActions()
    {
        return s_Actions;
    }

    const std::vector<InputAction> &InputActions::GetDefaults()
    {
        return s_Defaults;
    }

    void InputActions::SetDefaults(const std::vector<InputAction> &actions)
    {
        s_Defaults = actions;
        s_Actions = actions;
        s_Overridden.clear();
        LoadUserBindings();
    }

    bool InputActions::AddAction(const std::string &name)
    {
        if (name.empty() || Has(name))
            return false;
        InputAction action;
        action.Name = name;
        s_Actions.push_back(action);
        s_Defaults.push_back(action);
        return true;
    }

    bool InputActions::RemoveAction(const std::string &name)
    {
        const auto matches = [&](const InputAction &action)
        { return action.Name == name; };
        const size_t before = s_Actions.size();
        s_Actions.erase(std::remove_if(s_Actions.begin(), s_Actions.end(), matches), s_Actions.end());
        s_Defaults.erase(std::remove_if(s_Defaults.begin(), s_Defaults.end(), matches), s_Defaults.end());
        s_Overridden.erase(name);
        return s_Actions.size() != before;
    }

    std::vector<std::string> InputActions::GetBindingTexts(const std::string &name)
    {
        std::vector<std::string> result;
        if (const InputAction *action = Find(name))
            for (const InputBinding &binding : action->Bindings)
                result.push_back(ToText(binding));
        return result;
    }

    bool InputActions::Rebind(const std::string &name, const std::string &text)
    {
        InputAction *action = Find(name);
        auto binding = FromText(text);
        if (!action || !binding)
            return false;
        auto &list = action->Bindings;
        list.erase(std::remove_if(list.begin(), list.end(), [&](const InputBinding &existing)
                                  { return SameGroup(existing.Device, binding->Device); }),
                   list.end());
        list.push_back(*binding);
        s_Overridden.insert(name);
        SaveUserBindings();
        return true;
    }

    bool InputActions::AddBinding(const std::string &name, const std::string &text)
    {
        InputAction *action = Find(name);
        auto binding = FromText(text);
        if (!action || !binding)
            return false;
        if (std::find(action->Bindings.begin(), action->Bindings.end(), *binding) == action->Bindings.end())
            action->Bindings.push_back(*binding);
        s_Overridden.insert(name);
        SaveUserBindings();
        return true;
    }

    bool InputActions::RemoveBinding(const std::string &name, const std::string &text)
    {
        InputAction *action = Find(name);
        auto binding = FromText(text);
        if (!action || !binding)
            return false;
        auto &list = action->Bindings;
        const size_t before = list.size();
        list.erase(std::remove(list.begin(), list.end(), *binding), list.end());
        if (list.size() == before)
            return false;
        s_Overridden.insert(name);
        SaveUserBindings();
        return true;
    }

    bool InputActions::ClearBindings(const std::string &name)
    {
        InputAction *action = Find(name);
        if (!action)
            return false;
        action->Bindings.clear();
        s_Overridden.insert(name);
        SaveUserBindings();
        return true;
    }

    bool InputActions::ResetAction(const std::string &name)
    {
        InputAction *action = Find(name);
        if (!action)
            return false;
        for (const InputAction &original : s_Defaults)
            if (original.Name == name)
                action->Bindings = original.Bindings;
        s_Overridden.erase(name);
        SaveUserBindings();
        return true;
    }

    void InputActions::ResetAll()
    {
        for (InputAction &action : s_Actions)
            for (const InputAction &original : s_Defaults)
                if (original.Name == action.Name)
                    action.Bindings = original.Bindings;
        s_Overridden.clear();
        SaveUserBindings();
    }

    bool InputActions::IsOverridden(const std::string &name)
    {
        return s_Overridden.count(name) != 0;
    }

    std::string InputActions::ToText(const InputBinding &binding)
    {
        std::string text = binding.Scale < 0.0f ? "-" : "";
        switch (binding.Device)
        {
        case BindingDevice::Key:
        {
            const char *name = SDL_GetKeyName(static_cast<SDL_Keycode>(binding.Code));
            text += "Key.";
            text += (name && *name) ? name : std::to_string(binding.Code);
            break;
        }
        case BindingDevice::Mouse:
        {
            const char *name = NameOf(MouseButtons, binding.Code);
            text += "Mouse.";
            text += name ? name : std::to_string(binding.Code);
            break;
        }
        case BindingDevice::PadButton:
        {
            const char *name = NameOf(PadButtons, binding.Code);
            text += "Pad.";
            text += name ? name : std::to_string(binding.Code);
            break;
        }
        case BindingDevice::PadAxis:
        {
            const char *name = NameOf(PadAxes, binding.Code);
            text += "Axis.";
            text += name ? name : std::to_string(binding.Code);
            break;
        }
        }
        return text;
    }

    std::optional<InputBinding> InputActions::FromText(const std::string &input)
    {
        std::string text = input;
        float scale = 1.0f;
        if (!text.empty() && text[0] == '-')
        {
            scale = -1.0f;
            text.erase(0, 1);
        }
        const size_t dot = text.find('.');
        if (dot == std::string::npos || dot + 1 >= text.size())
            return std::nullopt;
        const std::string kind = text.substr(0, dot);
        const std::string name = text.substr(dot + 1);

        std::string lowerKind = kind;
        std::transform(lowerKind.begin(), lowerKind.end(), lowerKind.begin(), [](unsigned char c)
                       { return static_cast<char>(std::tolower(c)); });

        InputBinding binding;
        binding.Scale = scale;
        if (lowerKind == "key")
        {
            const SDL_Keycode key = SDL_GetKeyFromName(name.c_str());
            if (key == SDLK_UNKNOWN)
                return std::nullopt;
            binding.Device = BindingDevice::Key;
            binding.Code = static_cast<int>(key);
            return binding;
        }
        if (lowerKind == "mouse")
        {
            auto code = CodeOf(MouseButtons, name);
            if (!code)
                return std::nullopt;
            binding.Device = BindingDevice::Mouse;
            binding.Code = *code;
            return binding;
        }
        if (lowerKind == "pad")
        {
            auto code = CodeOf(PadButtons, name);
            if (!code)
                return std::nullopt;
            binding.Device = BindingDevice::PadButton;
            binding.Code = *code;
            return binding;
        }
        if (lowerKind == "axis")
        {
            auto code = CodeOf(PadAxes, name);
            if (!code)
                return std::nullopt;
            binding.Device = BindingDevice::PadAxis;
            binding.Code = *code;
            return binding;
        }
        return std::nullopt;
    }

    std::string InputActions::DisplayText(const InputBinding &binding)
    {
        std::string text = ToText(binding);
        const bool negative = !text.empty() && text[0] == '-';
        if (negative)
            text.erase(0, 1);
        const size_t dot = text.find('.');
        std::string name = dot == std::string::npos ? text : text.substr(dot + 1);
        if (binding.Device == BindingDevice::Mouse)
            name += " Mouse";
        else if (binding.Device == BindingDevice::PadButton)
            name = "Pad " + name;
        else if (binding.Device == BindingDevice::PadAxis)
            name = "Stick " + name;
        return negative ? name + " (-)" : name;
    }

    std::optional<std::string> InputActions::PollPressedBinding()
    {
        for (int key : Input::GetKeysJustPressed())
            return ToText({BindingDevice::Key, key, 1.0f});
        for (int button : Input::GetMouseButtonsJustPressed())
            return ToText({BindingDevice::Mouse, button, 1.0f});
        for (int button : Input::GetGamepadButtonsJustPressed())
            return ToText({BindingDevice::PadButton, button, 1.0f});
        for (const NameEntry &axis : PadAxes)
        {
            const float value = Input::GetGamepadAxis(axis.Code);
            if (std::abs(value) > 0.7f)
                return ToText({BindingDevice::PadAxis, axis.Code, value < 0.0f ? -1.0f : 1.0f});
        }
        return std::nullopt;
    }

    bool InputActions::LoadFile(const std::string &path)
    {
        const std::optional<std::string> text = FileSystem::ReadText(path);
        if (!text)
            return false;
        std::vector<InputAction> actions;
        if (!ParseActions(*text, actions))
            return false;
        s_Defaults = actions;
        s_Actions = actions;
        s_Overridden.clear();
        return true;
    }

    bool InputActions::SaveFile(const std::string &path)
    {
        YAML::Emitter out;
        EmitActions(out, s_Defaults);
        return FileSystem::WriteText(path, out.c_str());
    }

    void InputActions::LoadProject(const std::string &projectDirectory)
    {
        const std::string file = ProjectFile(projectDirectory);
        if (FileSystem::Exists(file) && LoadFile(file))
            s_ProjectLoaded = true;
        else
        {
            InstallBuiltinDefaults();
            s_ProjectLoaded = false;
        }
        LoadUserBindings();
    }

    bool InputActions::SaveProject(const std::string &projectDirectory)
    {
        if (!SaveFile(ProjectFile(projectDirectory)))
            return false;
        s_ProjectLoaded = true;
        return true;
    }

    void InputActions::CloseProject()
    {
        InstallBuiltinDefaults();
    }

    bool InputActions::IsProjectLoaded()
    {
        return s_ProjectLoaded;
    }

    void InputActions::EnableUserBindings(const std::string &gameName)
    {
        std::string safe = gameName;
        for (char &c : safe)
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-' && c != ' ')
                c = '_';
        if (safe.empty())
            safe = "Game";
        char *pref = SDL_GetPrefPath("SIMPEngine", safe.c_str());
        if (!pref)
            return;
        s_UserFile = FileSystem::Join(pref, "input-rebinds.yaml");
        SDL_free(pref);
        LoadUserBindings();
    }

    void InputActions::DisableUserBindings()
    {
        s_UserFile.clear();
    }

    void InputActions::LoadUserBindings()
    {
        if (s_UserFile.empty() || !FileSystem::Exists(s_UserFile))
            return;
        const std::optional<std::string> text = FileSystem::ReadText(s_UserFile);
        std::vector<InputAction> saved;
        if (!text || !ParseActions(*text, saved))
            return;
        for (InputAction &user : saved)
            if (InputAction *action = Find(user.Name))
            {
                action->Bindings = user.Bindings;
                s_Overridden.insert(user.Name);
            }
    }

    bool InputActions::SaveUserBindings()
    {
        if (s_UserFile.empty())
            return true;
        std::vector<InputAction> changed;
        for (const InputAction &action : s_Actions)
            if (s_Overridden.count(action.Name))
                changed.push_back(action);
        if (changed.empty())
        {
            FileSystem::Remove(s_UserFile);
            return true;
        }
        YAML::Emitter out;
        EmitActions(out, changed);
        return FileSystem::WriteText(s_UserFile, out.c_str());
    }
}
