#include <Engine/SIMPEngine.h>
#include "moveScript.h"

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

        // creation of entity
        SIMPEngine::Entity entity = scene1->CreateEntity("TestQuad");
        auto& transform = entity.GetComponent<TransformComponent>(); // x , y , scaleX, scaleY, rotation 
        transform.position = { 0, 0 };
        transform.scale = { 1, 1 }; 

        //its render component
        auto& render = entity.AddComponent<RenderComponent>();
        render.width = 200;
        render.height = 200;
        render.color = { 0, 255, 0, 255 };

        auto& cameraComp = entity.AddComponent<CameraComponent>();
        cameraComp.primary = false;

        auto & script = entity.AddComponent<ScriptComponent>();
        script.Bind<MoveRightScript>();

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
