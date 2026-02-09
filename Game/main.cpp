#include <Engine/SIMPEngine.h>
#include "ballScript.h"
#include "leftPlayerMoveScript.h"
#include "rightPlayerMoveScript.h"

#ifndef NO_EDITOR
#include "EditorLayer.h"
#endif

class MyGame : public SIMPEngine::Application
{
public:
    MyGame()
    {
        // make a sceneManager for swiching between scenes
        auto sceneManager = std::make_shared<SIMPEngine::SceneManager>();
        // make a scene 
        auto scene1 = std::make_shared<SIMPEngine::Scene>("Level1");

        // add the scene to scene manager
        sceneManager->AddScene("Level1", scene1);
        // now make it active, later when we change level we can make anohter level active
        sceneManager->SetActiveScene("Level1");
        

        SIMPEngine::SceneSerializer serializer(scene1.get());
        serializer.Deserialize("assets://scenes/Level1.yaml"); // this point everything parses

        serializer.Serialize("assets://scenes/Level2.yaml");
        SIMPEngine::RenderingLayer *m_RenderingLayer = new SIMPEngine::RenderingLayer(sceneManager);

#ifndef NO_EDITOR
        EditorLayer *m_EditorLayer = new EditorLayer(m_RenderingLayer);
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
