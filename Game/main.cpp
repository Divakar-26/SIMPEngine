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
        auto sceneManager = std::make_shared<SIMPEngine::SceneManager>();
        
        sceneManager->LoadScene("Level1", "assets://scenes/Level1.yaml");
        auto scene1 = sceneManager->GetActiveScene();

        scene1->BuildEntity("player").With<RenderComponent>(50,50).With<VelocityComponent>(1000,0).At(-1000,1000).Scale(1.0f, 1.0f).Build();

        SIMPEngine::SceneSerializer serializer(scene1.get());
        serializer.Serialize("assets://scenes/Level2.yaml");
        
        PushOverlay(new SIMPEngine::RenderingLayer(sceneManager));

#ifndef NO_EDITOR
        PushOverlay(new EditorLayer(sceneManager));
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
