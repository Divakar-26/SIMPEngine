#include "Application/Application.h"
#include "Core/Log.h"

#include "Layer/LayerStack.h"
#include "UI/ImGuiLayer.h"
#include "Layer/RenderingLayer.h"

#ifndef NO_EDITOR
#include "EditorLayer.h"
#endif

class MyGame : public SIMPEngine::Application
{
public:
    MyGame()
    {
        // // Custom game initialization
        SIMPEngine::RenderingLayer * m_RenderingLayer = new SIMPEngine::RenderingLayer();
        
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
