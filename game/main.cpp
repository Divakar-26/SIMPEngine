#include "Application/Application.h"
#include "Log.h"

#include "Layer/LayerStack.h"
#include "UI/ImGuiLayer.h"
#include "Layer/RenderingLayer.h"
#include "EditorLayer.h"

class MyGame : public SIMPEngine::Application
{
public:
    MyGame()
    {
        // Custom game initialization

        SIMPEngine::RenderingLayer * m_RenderingLayer = new SIMPEngine::RenderingLayer();
        EditorLayer * m_EditorLayer = new EditorLayer(m_RenderingLayer);

        PushOverlay(m_EditorLayer);
        PushOverlay(m_RenderingLayer);
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
