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

        SIMPEngine::SceneSerializer serializer(scene1.get());
        serializer.Deserialize("assets://scenes/Level1.yaml");

        auto entity = scene1->GetEntityByName("TestQuad");
        auto &script = entity.AddComponent<ScriptComponent>();
        script.Bind<MoveRightScript>();

        auto &phys = entity.AddComponent<PhysicsComponent>();
        phys.body = new AccelEngine::RigidBody();
        phys.body->position = {0, 0};
        phys.body->inverseMass = 1.0f;
        phys.body->shapeType = AccelEngine::ShapeType::AABB;
        phys.body->aabb.halfSize = {100, 100};
        phys.body->calculateInertia();
        phys.body->rotation = 3.0f;
        phys.body->velocity = {200.0f, 0.0f};
        scene1->physicsWorld.addBody(phys.body);
        scene1->bodies.push_back(phys.body);

        auto entity2 = scene1->GetEntityByName("TestQuad2");
        auto &phys2 = entity2.AddComponent<PhysicsComponent>(); 
        phys2.body = new AccelEngine::RigidBody();
        phys2.body->position = {700, 0};
        phys2.body->inverseMass = 0.0f;
        phys2.body->shapeType = AccelEngine::ShapeType::AABB;
        phys2.body->aabb.halfSize = {100, 100};
        phys2.body->calculateInertia();
        phys2.body->rotation = 0.0f;
        phys2.body->velocity = {0.0f, 0.0f};
        scene1->physicsWorld.addBody(phys2.body);
        scene1->bodies.push_back(phys2.body);   

        // serializer.Serialize("assets://scenes/Level1.yaml");

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
