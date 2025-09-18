#include "Application/Application.h"
#include "Core/Log.h"

#include "Layer/LayerStack.h"
#include "UI/ImGuiLayer.h"
#include "Layer/RenderingLayer.h"
#include "Scene/SceneManager.h"

#ifndef NO_EDITOR
#include "EditorLayer.h"
#endif

class MyGame : public SIMPEngine::Application
{
public:
    MyGame()
    {
        
        auto sceneManager = std::make_shared<SIMPEngine::SceneManager>();
        auto scene1 = std::make_shared<SIMPEngine::Scene>("Level1");
        sceneManager->AddScene("Level1", scene1);
        sceneManager->SetActiveScene("Level1");

        SIMPEngine::RenderingLayer * m_RenderingLayer = new SIMPEngine::RenderingLayer(sceneManager);
        
        #ifndef NO_EDITOR
            EditorLayer * m_EditorLayer = new EditorLayer(m_RenderingLayer);
            PushOverlay(m_EditorLayer);
        #else
            PushOverlay(m_RenderingLayer); 
        #endif
    }
};

SIMPEngine::Application *SIMPEngine::CreateApplication()
{
    return new MyGame();
}

int main()
{
    SIMPEngine::Log::Init();
    CORE_INFO("Engine Starting up....");
    auto app = SIMPEngine::CreateApplication();
    app->Run();
    delete app;
}
