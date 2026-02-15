#pragma once

#include <Engine/Scene/Scene.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace SIMPEngine
{
    class SceneManager
    {
    public:
        SceneManager() = default;
        ~SceneManager() = default;

        void AddScene(const std::string & name, std::shared_ptr<Scene> scene)
        {
            m_Scenes[name] = std::move(scene);
        }

        void RemoveScene(const std::string & name)
        {
            auto it = m_Scenes.find(name);
            if(it == m_Scenes.end()){
                return;
            }

            if(m_ActiveScene == it->second){
                m_ActiveScene.reset();
            }

            m_Scenes.erase(it);
        }

        void SetActiveScene(const std::string & name)
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

        const std::unordered_map<std::string, std::shared_ptr<Scene>> &GetAllScenes() const
        {
            return m_Scenes;
        }

    private:
        std::unordered_map<std::string, std::shared_ptr<Scene>> m_Scenes;
        std::shared_ptr<Scene> m_ActiveScene;
    };
}