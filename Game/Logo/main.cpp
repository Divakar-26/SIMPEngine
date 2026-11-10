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
        spec.Title = "SIMPEngine";
        spec.Width = 1200;
        spec.Height = 540;
        return spec;
    }
}

class LogoDemo : public SIMPEngine::Application
{
public:
    LogoDemo()
        : Application(Specification())
    {
        auto sceneManager = std::make_shared<SIMPEngine::SceneManager>();
        auto scene = std::make_shared<SIMPEngine::Scene>("Logo");
        scene->SetBackgroundColor({1.0f, 0.976f, 0.839f, 1.0f});
        sceneManager->AddScene("Logo", scene);
        sceneManager->SetActiveScene("Logo");

        auto camera = scene->CreateEntity("Camera");
        auto &cc = camera.AddComponent<CameraComponent>(0.7f, glm::vec2(0.0f, 0.0f));
        cc.primary = true;
        cc.Camera.SetManualControl(false);

        auto logo = scene->CreateEntity("Logo");
        logo.AddComponent<ScriptComponent>().BindLua("assets://scripts/logo.lua");

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
    return new LogoDemo();
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
