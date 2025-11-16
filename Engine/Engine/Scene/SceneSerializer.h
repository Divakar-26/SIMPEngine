#pragma once
#include <Engine/Scene/Scene.h>

#include <yaml-cpp/yaml.h>

namespace SIMPEngine
{
    class SceneSerializer
    {
    public:
        SceneSerializer(Scene* scene) : m_Scene(scene) {}

        bool Serialize(const std::string& filepath);
        bool Deserialize(const std::string& filepath);

    private:
        Scene* m_Scene;
    };
}
