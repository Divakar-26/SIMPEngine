#pragma once
#include <Engine/Scene/Scene.h>
#include <Engine/Core/Log.h>
#include <yaml-cpp/yaml.h>

#include <string>
#include <vector>

namespace SIMPEngine
{
    class SceneSerializer
    {
    public:
        SceneSerializer(Scene *scene) : m_Scene(scene) {}

        bool Serialize(const std::string &filepath);
        bool Deserialize(const std::string &filepath);

        std::string SerializeToString();
        bool DeserializeFromString(const std::string &yaml, std::vector<entt::entity> *created = nullptr);

        std::string SerializeEntities(const std::vector<entt::entity> &entities);
        std::vector<entt::entity> DeserializeEntities(const std::string &yaml);

    private:
        std::string Emit(const std::vector<entt::entity> &entities, bool includeSettings);
        bool Load(const YAML::Node &data, bool applySettings, std::vector<entt::entity> *created);

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
