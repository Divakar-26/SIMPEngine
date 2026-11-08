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

class ScriptDemo : public SIMPEngine::Application
{
public:
    ScriptDemo()
    {
        auto sceneManager = std::make_shared<SIMPEngine::SceneManager>();
        auto scene = std::make_shared<SIMPEngine::Scene>("Script Demo");
        sceneManager->AddScene("Script Demo", scene);
        sceneManager->SetActiveScene("Script Demo");

        auto camera = scene->CreateEntity("Camera");
        auto &cc = camera.AddComponent<CameraComponent>(0.6f, glm::vec2(0.0f, 0.0f));
        cc.primary = true;
        cc.followTarget = "Player";
        cc.followSmoothing = 5.0f;
        cc.useBounds = true;
        cc.boundsMin = {-800.0f, -450.0f};
        cc.boundsMax = {800.0f, 450.0f};
        cc.shakeStrength = 18.0f;

        auto demo = scene->CreateEntity("Demo");
        demo.AddComponent<ScriptComponent>().BindLua("assets://scripts/demo/main.lua");

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
    return new ScriptDemo();
}

int main()
{
    SIMPEngine::Log::Init();
    CORE_INFO("Script demo starting up....");
    SIMPEngine::VFS::Mount("assets", "../assets");
    auto *app = SIMPEngine::CreateApplication();
    app->Run();
    delete app;
    return 0;
}
