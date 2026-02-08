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

        auto entity = scene1->GetEntityByName("Ball");
        auto &ballRender = entity.GetComponent<RenderComponent>();
        auto &ballTransform = entity.GetComponent<TransformComponent>();
        auto &script = entity.AddComponent<ScriptComponent>();
        script.Bind<BallScript>();

        auto &phys = entity.AddComponent<PhysicsComponent>();
        phys.body = new AccelEngine::RigidBody();
        phys.body->position = {ballTransform.position.x, ballTransform.position.y};
        phys.body->inverseMass = 1.0f;
        phys.body->shapeType = AccelEngine::ShapeType::CIRCLE;
        phys.body->circle.radius = {ballRender.width / 2};
        phys.body->calculateInertia();
        phys.body->rotation = 0.0f;
        phys.body->velocity = {500.0f, 200.0f};
        phys.body->restitution = 1.0f;
        phys.body->staticFriction = 0.0f;
        phys.body->dynamicFriction = 0.0f;
        phys.body->lockRotation = true;
        scene1->physicsWorld.addBody(phys.body);
        scene1->bodies.push_back(phys.body);

        auto leftPlayer = scene1->GetEntityByName("LeftPlayer");
        auto &leftPhys = leftPlayer.AddComponent<PhysicsComponent>();
        auto &leftPlayerScript = leftPlayer.AddComponent<ScriptComponent>();
        auto &leftRender = leftPlayer.GetComponent<RenderComponent>();
        auto &leftTransform = leftPlayer.GetComponent<TransformComponent>();
        leftPlayerScript.Bind<LeftPlayerMove>();
        leftPhys.body = new AccelEngine::RigidBody();
        leftPhys.body->position = {leftTransform.position.x, leftTransform.position.y};
        leftPhys.body->inverseMass = 0.0;
        leftPhys.body->shapeType = AccelEngine::ShapeType::AABB;
        leftPhys.body->aabb.halfSize = {leftRender.width / 2, leftRender.height / 2};
        leftPhys.body->calculateInertia();
        leftPhys.body->rotation = 0.0f;
        leftPhys.body->restitution = 1.0f;
        leftPhys.body->staticFriction = 0.0f;
        leftPhys.body->dynamicFriction = 0.0f;
        // leftPhys.body->lockPosition = true;
        // leftPhys.body->lockRotation = true;
        scene1->physicsWorld.addBody(leftPhys.body);
        scene1->bodies.push_back(leftPhys.body);

        auto rigthPlayer = scene1->GetEntityByName("RightPlayer");
        auto &rightPhys = rigthPlayer.AddComponent<PhysicsComponent>();
        auto &rigthPlayerScript = rigthPlayer.AddComponent<ScriptComponent>();
        auto &rightRender = rigthPlayer.GetComponent<RenderComponent>();
        auto &rightTransform = rigthPlayer.GetComponent<TransformComponent>();
        rigthPlayerScript.Bind<RightPlayerMove>();
        rightPhys.body = new AccelEngine::RigidBody();
        rightPhys.body->position = {rightTransform.position.x, rightTransform.position.y};
        rightPhys.body->inverseMass = 0.000001f;
        rightPhys.body->shapeType = AccelEngine::ShapeType::AABB;
        rightPhys.body->aabb.halfSize = {rightRender.width / 2, rightRender.height / 2};
        rightPhys.body->calculateInertia();
        rightPhys.body->rotation = 0.0f;
        rightPhys.body->restitution = 1.0f;
        rightPhys.body->staticFriction = 0.0f;
        rightPhys.body->dynamicFriction = 0.0f;
        scene1->physicsWorld.addBody(rightPhys.body);
        scene1->bodies.push_back(rightPhys.body);

        
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
