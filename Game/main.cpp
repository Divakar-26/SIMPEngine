#include <Engine/PCH.h>
#include "ConstellationScript.h"

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

        auto constellation = scene1->BuildEntity("constellation")
                                 .At(0, 0)
                                 .Build();

        // Get the entity and bind the script
        auto &sc = scene1->GetRegistry().emplace<ScriptComponent>(constellation);
        sc.Bind<ConstellationScript>();

        SIMPEngine::SceneSerializer serializer(scene1.get());
        serializer.Serialize("assets://scenes/Level2.yaml");

        auto renderingLayer = new SIMPEngine::RenderingLayer(sceneManager);
        PushOverlay(renderingLayer);

#ifndef NO_EDITOR
        PushOverlay(new EditorLayer(renderingLayer));
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
    SIMPEngine::VFS::Mount("assets", "../assets");
    CORE_INFO("Mounted assets VFS at {}", "../assets");
    auto app = SIMPEngine::CreateApplication();
    app->Run();
    delete app;
}
