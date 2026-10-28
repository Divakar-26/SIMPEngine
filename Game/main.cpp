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
        sceneManager->LoadScene(
            "Level1",
            "assets://scenes/Level1.yaml"
        );
        auto scene1 = sceneManager->GetActiveScene();
        auto constellation =
            scene1->BuildEntity("constellation")
                .At(0, 0)
                .Build();

        // Bind the script to the entity.
        auto &sc =
            scene1->GetRegistry().emplace<ScriptComponent>(
                constellation
            );
        sc.Bind<ConstellationScript>();
        SIMPEngine::SceneSerializer serializer(scene1.get());
        serializer.Serialize("assets://scenes/Level2.yaml");
        // --------------------------------------------------------
        // Rendering layer
        //
        // LayerStack now OWNS layers through unique_ptr.
        // We temporarily keep a raw pointer because EditorLayer
        // needs to reference the RenderingLayer but must NOT own it.
        // --------------------------------------------------------
        auto renderingLayer =
            std::make_unique<SIMPEngine::RenderingLayer>(
                sceneManager
            );

        SIMPEngine::RenderingLayer *renderingLayerPtr =
            renderingLayer.get();

        PushOverlay(std::move(renderingLayer));

#ifndef NO_EDITOR

        // EditorLayer does not own RenderingLayer.
        // LayerStack owns both layers.
        PushOverlay(
            std::make_unique<EditorLayer>(
                renderingLayerPtr
            )
        );

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
    SIMPEngine::VFS::Mount(
        "assets",
        "../assets"
    );
    CORE_INFO(
        "Mounted assets VFS at {}",
        "../assets"
    );
    auto *app = SIMPEngine::CreateApplication();
    app->Run();
    delete app;
    return 0;
}