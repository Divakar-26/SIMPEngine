#pragma once
#include <Engine/Scene/Scene.h>
#include <Engine/Core/Log.h>
#include <yaml-cpp/yaml.h>

namespace SIMPEngine
{
    class SceneSerializer
    {
    public:
        SceneSerializer(Scene *scene) : m_Scene(scene) {}

        bool Serialize(const std::string &filepath);
        bool Deserialize(const std::string &filepath);

    private:
        Scene *m_Scene;
    };

    template <typename T>
    bool TryGet(YAML::Node node, const char *key, T &out)
    {
        if (!node[key])
        {
            CORE_ERROR("Missing key '{}'", key);
            return false;
        }

        try
        {
            out = node[key].as<T>();
            return true;
        }
        catch (const YAML::Exception &e)
        {
            CORE_ERROR("Invalid type for key '{}': {}", key, e.what());
            return false;
        }
    }

}
