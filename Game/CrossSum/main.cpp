#include <Engine/PCH.h>
#include <Engine/Application/Application.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneManager.h>

#ifndef NO_EDITOR
#include "EditorLayer.h"
#endif

namespace
{
    SIMPEngine::ApplicationSpecification Specification()
    {
        SIMPEngine::ApplicationSpecification spec;
        spec.Title = "Cross Sum";
        spec.Width = 1600;
        spec.Height = 900;
        return spec;
    }
}

class CrossSumDemo : public SIMPEngine::Application
{
public:
    CrossSumDemo()
        : Application(Specification())
    {
        auto sceneManager = std::make_shared<SIMPEngine::SceneManager>();
        auto scene = std::make_shared<SIMPEngine::Scene>("CrossSum");
        scene->SetBackgroundColor({0.04f, 0.05f, 0.15f, 1.0f});
        sceneManager->AddScene("CrossSum", scene);
        sceneManager->SetActiveScene("CrossSum");

        auto camera = scene->CreateEntity("Camera");
        auto &cc = camera.AddComponent<CameraComponent>(1.0f, glm::vec2(0.0f, 0.0f));
        cc.primary = true;
        cc.Camera.SetManualControl(false);

        auto game = scene->CreateEntity("Game");
        game.AddComponent<ScriptComponent>().BindLua("assets://scripts/crosssum/main.lua");

        auto renderingLayer = std::make_unique<SIMPEngine::RenderingLayer>(sceneManager);
        SIMPEngine::RenderingLayer *renderingLayerPtr = renderingLayer.get();
        PushOverlay(std::move(renderingLayer));

#ifndef NO_EDITOR
        PushOverlay(std::make_unique<EditorLayer>(renderingLayerPtr));
#else
        (void)renderingLayerPtr;
#endif
    }
};

SIMPEngine::Application *SIMPEngine::CreateApplication()
{
    return new CrossSumDemo();
}

int main()
{
    SIMPEngine::Log::Init();
    SIMPEngine::VFS::Mount("assets", "../assets");
    auto *app = SIMPEngine::CreateApplication();
    app->Run();
    delete app;
    return 0;
}
