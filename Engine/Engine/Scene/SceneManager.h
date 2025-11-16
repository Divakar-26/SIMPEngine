#pragma once

#include <Engine/Scene/Scene.h>

#include <memory>
#include <unordered_map>
#include <string>

namespace SIMPEngine
{
    class SceneManager
    {
    public:
        SceneManager() = default;
        ~SceneManager() = default;

        void AddScene(const std::string name, std::shared_ptr<Scene> scene)
        {
            m_Scenes[name] = scene;
        }

        void RemoveScene(const std::string name)
        {
            if (m_ActiveScene && m_ActiveScene == m_Scenes[name])
            {
                m_ActiveScene.reset();
            }

            m_Scenes.erase(name);
        }

        void SetActiveScene(const std::string name)
        {
            auto it = m_Scenes.find(name);
            if (it != m_Scenes.end())
            {
                m_ActiveScene = it->second;
            }
        }

        std::shared_ptr<Scene> GetActiveScene() { return m_ActiveScene; }
        std::shared_ptr<const Scene> GetActiveScene() const { return m_ActiveScene; }

        bool HasScene(const std::string name) const
        {
            return m_Scenes.find(name) != m_Scenes.end();
        }

        const std::unordered_map<std::string, std::shared_ptr<Scene>>& GetAllScenes() const
        {
            return m_Scenes;
        }

    private:
        std::unordered_map<std::string, std::shared_ptr<Scene>> m_Scenes;
        std::shared_ptr<Scene> m_ActiveScene;
    };
}