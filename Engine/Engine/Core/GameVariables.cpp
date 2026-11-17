#include <Engine/Core/GameVariables.h>
#include <Engine/Core/FileSystem.h>
#include <Engine/Core/Log.h>
#include <Engine/Project/Project.h>

#include <SDL3/SDL.h>
#include <yaml-cpp/yaml.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <cctype>
#include <optional>
#include <utility>
#include <exception>

namespace SIMPEngine
{
    namespace
    {
        std::map<std::string, double> s_Values;
        std::map<std::string, std::string> s_Texts;

        std::string FormatNumber(double value)
        {
            char buffer[48];
            if (std::abs(value - std::round(value)) < 1e-9 && std::abs(value) < 1e15)
                std::snprintf(buffer, sizeof(buffer), "%lld", static_cast<long long>(std::llround(value)));
            else
                std::snprintf(buffer, sizeof(buffer), "%.2f", value);
            return buffer;
        }

        std::string SlotPath(const std::string &slot)
        {
            std::string safe;
            for (char c : slot)
                safe.push_back((std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-') ? c : '_');
            if (safe.empty())
                safe = "save";

            std::string game = Project::IsOpen() ? Project::Settings().Name : std::string("Game");
            for (char &c : game)
                if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-' && c != ' ')
                    c = '_';
            if (game.empty())
                game = "Game";
            if (const char *override = std::getenv("SIMP_SAVE_DIR"); override && *override)
            {
                std::error_code ec;
                std::filesystem::create_directories(override, ec);
                return FileSystem::Join(override, safe + ".sav");
            }
            char *pref = SDL_GetPrefPath("SIMPEngine", game.c_str());
            if (!pref)
                return {};
            std::string path = FileSystem::Join(pref, safe + ".sav");
            SDL_free(pref);
            return path;
        }
    }

    double GameVariables::Get(const std::string &key, double fallback)
    {
        auto it = s_Values.find(key);
        return it == s_Values.end() ? fallback : it->second;
    }

    void GameVariables::Set(const std::string &key, double value)
    {
        if (!key.empty())
            s_Values[key] = value;
    }

    double GameVariables::Add(const std::string &key, double amount)
    {
        if (key.empty())
            return 0.0;
        return s_Values[key] += amount;
    }

    bool GameVariables::Has(const std::string &key)
    {
        return s_Values.find(key) != s_Values.end();
    }

    void GameVariables::Remove(const std::string &key)
    {
        s_Values.erase(key);
    }

    void GameVariables::Clear()
    {
        s_Values.clear();
        s_Texts.clear();
    }

    std::string GameVariables::GetText(const std::string &key, const std::string &fallback)
    {
        auto it = s_Texts.find(key);
        return it == s_Texts.end() ? fallback : it->second;
    }

    void GameVariables::SetText(const std::string &key, const std::string &value)
    {
        if (!key.empty())
            s_Texts[key] = value;
    }

    bool GameVariables::HasText(const std::string &key)
    {
        return s_Texts.find(key) != s_Texts.end();
    }

    void GameVariables::RemoveText(const std::string &key)
    {
        s_Texts.erase(key);
    }

    const std::map<std::string, double> &GameVariables::All()
    {
        return s_Values;
    }

    void GameVariables::Replace(std::map<std::string, double> values)
    {
        s_Values = std::move(values);
    }

    std::string GameVariables::Format(const std::string &text)
    {
        if (text.find('{') == std::string::npos)
            return text;

        std::string out;
        out.reserve(text.size());
        for (size_t i = 0; i < text.size(); ++i)
        {
            if (text[i] == '{')
            {
                const size_t close = text.find('}', i + 1);
                if (close != std::string::npos)
                {
                    const std::string key = text.substr(i + 1, close - i - 1);
                    if (auto it = s_Values.find(key); it != s_Values.end())
                    {
                        out += FormatNumber(it->second);
                        i = close;
                        continue;
                    }
                    if (auto it = s_Texts.find(key); it != s_Texts.end())
                    {
                        out += it->second;
                        i = close;
                        continue;
                    }
                }
            }
            out.push_back(text[i]);
        }
        return out;
    }

    bool SaveData::Save(const std::string &slot)
    {
        const std::string path = SlotPath(slot);
        if (path.empty())
            return false;

        YAML::Emitter out;
        out << YAML::BeginMap << YAML::Key << "Variables" << YAML::Value << YAML::BeginMap;
        for (const auto &[key, value] : s_Values)
            out << YAML::Key << key << YAML::Value << value;
        out << YAML::EndMap;
        if (!s_Texts.empty())
        {
            out << YAML::Key << "Text" << YAML::Value << YAML::BeginMap;
            for (const auto &[key, value] : s_Texts)
                out << YAML::Key << key << YAML::Value << YAML::DoubleQuoted << value;
            out << YAML::EndMap;
        }
        out << YAML::EndMap;

        if (!FileSystem::WriteText(path, out.c_str()))
        {
            CORE_ERROR("Save: could not write {}", path);
            return false;
        }
        return true;
    }

    bool SaveData::Load(const std::string &slot)
    {
        const std::string path = SlotPath(slot);
        const auto text = path.empty() ? std::nullopt : FileSystem::ReadText(path);
        if (!text)
            return false;

        try
        {
            const YAML::Node root = YAML::Load(*text);
            const YAML::Node variables = root["Variables"];
            if (!variables || !variables.IsMap())
                return false;
            std::map<std::string, double> loaded;
            for (const auto &entry : variables)
                loaded[entry.first.as<std::string>()] = entry.second.as<double>();
            std::map<std::string, std::string> loadedText;
            if (const YAML::Node texts = root["Text"]; texts && texts.IsMap())
                for (const auto &entry : texts)
                    loadedText[entry.first.as<std::string>()] = entry.second.as<std::string>();
            s_Values = std::move(loaded);
            s_Texts = std::move(loadedText);
            return true;
        }
        catch (const std::exception &e)
        {
            CORE_ERROR("Save: '{}' is damaged: {}", path, e.what());
            return false;
        }
    }

    bool SaveData::Exists(const std::string &slot)
    {
        const std::string path = SlotPath(slot);
        return !path.empty() && FileSystem::Exists(path);
    }

    bool SaveData::Delete(const std::string &slot)
    {
        const std::string path = SlotPath(slot);
        return !path.empty() && FileSystem::Remove(path);
    }
}
