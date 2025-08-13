#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include "Scene.h"

namespace SIMPEngine
{
    class SceneManager
    {
    public:
        template<typename... Args>
        Scene& CreateScene(const std::string& name, Args&&... args)
        {
            auto scene = std::make_shared<Scene>(name, std::forward<Args>(args)...);
            m_Scenes[name] = scene;
            return *scene;
        }

        void SetActiveScene(const std::string& name)
        {
            if (m_Scenes.count(name))
                m_ActiveScene = m_Scenes[name];
        }

        Scene* GetActiveScene() { return m_ActiveScene.get(); }

    private:
        std::unordered_map<std::string, std::shared_ptr<Scene>> m_Scenes;
        std::shared_ptr<Scene> m_ActiveScene;
    };
}
