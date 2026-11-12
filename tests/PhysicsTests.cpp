#include "TestFramework.h"

#include <Engine/Core/GameVariables.h>
#include <Engine/Core/VFS.h>
#include <Engine/Physics/BodyFactory.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneSerializer.h>
#include <Engine/Scene/ScriptableEntity.h>

#include <AccelEngine/world.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace SIMPEngine;
namespace fs = std::filesystem;

namespace
{
    void Run(Scene &scene, int frames)
    {
        for (int i = 0; i < frames; ++i)
            scene.OnUpdate(1.0f / 60.0f);
    }

    Entity MakeGround(Scene &scene, float y = 0.0f)
    {
        Entity ground = scene.CreateEntity("Ground");
        ground.GetComponent<TransformComponent>().position = {0.0f, y};
        PhysicsBodies::Options options;
        options.Mass = 0.0f;
        scene.AddBody(ground.GetHandle(), PhysicsBodies::Box({400.0f, 20.0f}, options));
        return ground;
    }

    Entity MakeBall(Scene &scene, glm::vec2 at, float radius = 20.0f, const char *name = "Ball")
    {
        Entity ball = scene.CreateEntity(name);
        ball.GetComponent<TransformComponent>().position = at;
        PhysicsBodies::Options options;
        options.Restitution = 0.0f;
        scene.AddBody(ball.GetHandle(), PhysicsBodies::Circle(radius, options));
        return ball;
    }

    struct Probe : ScriptableEntity
    {
        static inline int Enter = 0, Stay = 0, Exit = 0, Hits = 0;
        static void Reset() { Enter = Stay = Exit = Hits = 0; }

        void OnTriggerEnter(Entity) override { ++Enter; }
        void OnTriggerStay(Entity) override { ++Stay; }
        void OnTriggerExit(Entity) override { ++Exit; }
        void OnCollision(Entity) override { ++Hits; }
    };
}

TEST_CASE("physics: bodies fall under gravity, rest on the ground and fall asleep")
{
    Scene scene("Gravity");
    MakeGround(scene);
    Entity ball = MakeBall(scene, {0.0f, 240.0f});
    const auto &position = ball.GetComponent<TransformComponent>().position;

    Run(scene, 10);
    CHECK(position.y < 240.0f);

    Run(scene, 300);
    CHECK_NEAR(position.y, 40.0f, 3.0);
    CHECK(!ball.GetComponent<PhysicsComponent>().body->isAwake);
}

TEST_CASE("physics: gravity is a scene setting")
{
    Scene scene("Gravity setting");
    scene.GetPhysicsWorld().gravity = {0.0f, 0.0f};
    Entity ball = MakeBall(scene, {0.0f, 100.0f});
    Run(scene, 60);
    CHECK_NEAR(ball.GetComponent<TransformComponent>().position.y, 100.0f, 0.01);

    scene.GetPhysicsWorld().gravity = {0.0f, -980.0f};
    scene.GetPhysicsWorld().wakeAll();
    Run(scene, 30);
    CHECK(ball.GetComponent<TransformComponent>().position.y < 90.0f);
}

TEST_CASE("physics: moving a transform from anywhere moves the body")
{
    Scene scene("Push");
    scene.GetPhysicsWorld().gravity = {0.0f, 0.0f};
    Entity ball = MakeBall(scene, {0.0f, 0.0f});
    Run(scene, 2);

    ball.GetComponent<TransformComponent>().position = {500.0f, 30.0f};
    Run(scene, 1);
    const auto &body = *ball.GetComponent<PhysicsComponent>().body;
    CHECK_NEAR(body.position.x, 500.0f, 0.01);
    CHECK_NEAR(body.position.y, 30.0f, 0.01);
    CHECK_NEAR(ball.GetComponent<TransformComponent>().position.x, 500.0f, 0.01);

    ball.GetComponent<TransformComponent>().rotation = 90.0f;
    Run(scene, 1);
    CHECK_NEAR(body.orientation, 1.5707963f, 1e-3);
}

TEST_CASE("physics: a static body follows its transform and wakes what rests on it")
{
    Scene scene("Platform");
    Entity platform = MakeGround(scene, 0.0f);
    Entity ball = MakeBall(scene, {0.0f, 40.0f});
    Run(scene, 240);
    CHECK(!ball.GetComponent<PhysicsComponent>().body->isAwake);

    platform.GetComponent<TransformComponent>().position = {0.0f, -100.0f};
    Run(scene, 240);
    CHECK_NEAR(platform.GetComponent<PhysicsComponent>().body->position.y, -100.0f, 0.01);
    CHECK_NEAR(ball.GetComponent<TransformComponent>().position.y, -60.0f, 3.0);
}

TEST_CASE("physics: a child body is written back in its parent's space")
{
    Scene scene("Child");
    scene.GetPhysicsWorld().gravity = {0.0f, 0.0f};
    Entity parent = scene.CreateEntity("Parent");
    parent.GetComponent<TransformComponent>().position = {100.0f, 50.0f};
    Entity child = scene.CreateEntity("Child");
    child.GetComponent<TransformComponent>().position = {10.0f, 0.0f};
    scene.SetParent(child.GetHandle(), parent.GetHandle());
    PhysicsBodies::Options options;
    scene.AddBody(child.GetHandle(), PhysicsBodies::Circle(5.0f, options));

    Run(scene, 2);
    auto &body = *child.GetComponent<PhysicsComponent>().body;
    CHECK_NEAR(body.position.x, 110.0f, 0.01);
    CHECK_NEAR(body.position.y, 50.0f, 0.01);

    body.setVelocity({60.0f, 0.0f}, 0.0f);
    Run(scene, 60);
    CHECK_NEAR(child.GetComponent<TransformComponent>().position.x, 70.0f, 1.5);
    CHECK_NEAR(child.GetComponent<TransformComponent>().position.y, 0.0f, 0.05);
}

TEST_CASE("physics: sensors raise trigger events, solid contacts raise collisions")
{
    Probe::Reset();
    Scene scene("Sensors");
    MakeGround(scene);

    Entity zone = scene.CreateEntity("Zone");
    zone.GetComponent<TransformComponent>().position = {0.0f, 120.0f};
    PhysicsBodies::Options staticOptions;
    staticOptions.Mass = 0.0f;
    auto sensor = PhysicsBodies::Box({60.0f, 10.0f}, staticOptions);
    sensor->isSensor = true;
    scene.AddBody(zone.GetHandle(), std::move(sensor));
    zone.AddComponent<ScriptComponent>().Bind<Probe>();

    MakeBall(scene, {0.0f, 260.0f});
    Run(scene, 240);

    CHECK(Probe::Enter == 1);
    CHECK(Probe::Exit == 1);
    CHECK(Probe::Hits == 0);
}

TEST_CASE("physics: touching bodies report OnCollision")
{
    Probe::Reset();
    Scene scene("Contacts");
    Entity ground = MakeGround(scene);
    ground.AddComponent<ScriptComponent>().Bind<Probe>();
    MakeBall(scene, {0.0f, 80.0f});
    Run(scene, 120);
    CHECK(Probe::Hits > 0);
    CHECK(Probe::Enter == 0);
}

TEST_CASE("physics: collision layers decide who collides")
{
    Scene scene("Layers");
    MakeGround(scene);
    Entity ghost = MakeBall(scene, {0.0f, 100.0f}, 20.0f, "Ghost");
    ghost.GetComponent<PhysicsComponent>().body->maskBits = 0x0002;
    Entity solid = MakeBall(scene, {200.0f, 100.0f}, 20.0f, "Solid");

    Run(scene, 240);
    CHECK(ghost.GetComponent<TransformComponent>().position.y < -50.0f);
    CHECK_NEAR(solid.GetComponent<TransformComponent>().position.y, 40.0f, 3.0);
}

TEST_CASE("physics: removing bodies and destroying entities while others simulate is safe")
{
    Scene scene("Removal");
    MakeGround(scene);
    std::vector<Entity> balls;
    for (int i = 0; i < 40; ++i)
        balls.push_back(MakeBall(scene, {static_cast<float>(i * 8 - 150), 60.0f + static_cast<float>(i * 12)}, 6.0f, "Ball"));
    Run(scene, 20);
    CHECK(scene.GetPhysicsWorld().getBodyCount() == 41);

    for (size_t i = 0; i < balls.size(); i += 3)
        scene.DestroyEntity(balls[i]);
    for (size_t i = 1; i < balls.size(); i += 3)
        scene.RemoveBody(balls[i].GetHandle());
    CHECK(scene.GetPhysicsWorld().getBodyCount() == 1 + 13);
    Run(scene, 60);

    scene.Clear();
    CHECK(scene.GetPhysicsWorld().getBodyCount() == 0);
    Run(scene, 5);
}

TEST_CASE("physics: creating and destroying many worlds never hangs")
{
    for (int i = 0; i < 300; ++i)
    {
        AccelEngine::World world;
        AccelEngine::BodyDef def;
        def.position = {0.0f, 10.0f};
        world.createCircle(5.0f, def);
        if (i % 2 == 0)
            world.step(1.0f / 60.0f);
    }
    CHECK(true);
}

TEST_CASE("physics: every shape survives save and load")
{
    Scene original("Shapes");
    original.GetPhysicsWorld().gravity = {12.0f, -300.0f};
    original.GetPhysicsWorld().settings.substeps = 8;
    original.GetPhysicsWorld().settings.enableSleep = false;

    auto add = [&](const char *name, std::unique_ptr<AccelEngine::RigidBody> body)
    {
        Entity e = original.CreateEntity(name);
        original.AddBody(e.GetHandle(), std::move(body));
        return e;
    };
    PhysicsBodies::Options options;
    options.Mass = 3.0f;
    options.Restitution = 0.7f;

    auto circle = PhysicsBodies::Circle(12.0f, options);
    circle->categoryBits = 0x0004;
    circle->maskBits = 0x00F1;
    circle->groupIndex = -2;
    circle->isSensor = true;
    circle->linearDamping = 0.8f;
    circle->angularDamping = 0.6f;
    circle->ignoreGravity = true;
    circle->canSleep = false;
    circle->rotation = 1.25f;
    circle->velocity = {4.0f, -5.0f};
    Entity circleEntity = add("Circle", std::move(circle));
    circleEntity.GetComponent<TransformComponent>().rotation = 28.6478897f;
    circleEntity.GetComponent<TransformComponent>().position = {70.0f, -20.0f};

    add("Box", PhysicsBodies::Box({30.0f, 8.0f}, options));
    auto capsule = PhysicsBodies::Capsule(25.0f, 9.0f, options);
    capsule->oneWay = true;
    capsule->oneWayNormal = {1.0f, 0.0f};
    add("Capsule", std::move(capsule));
    add("Hexagon", PhysicsBodies::RegularPolygon(6, 20.0f, options));

    std::vector<AccelEngine::ConvexPart> parts = {AccelEngine::ConvexPart::box({20.0f, 5.0f}), AccelEngine::ConvexPart::circle({25.0f, 10.0f}, 7.0f)};
    add("Compound", PhysicsBodies::FromParts(AccelEngine::ShapeType::COMPOUND, parts, options, true));

    const std::string yaml = SceneSerializer(&original).SerializeToString();
    Scene loaded("Loaded");
    CHECK(SceneSerializer(&loaded).DeserializeFromString(yaml));
    CHECK(SceneSerializer(&loaded).SerializeToString() == yaml);

    CHECK_NEAR(loaded.GetPhysicsWorld().gravity.x, 12.0f, 1e-4);
    CHECK_NEAR(loaded.GetPhysicsWorld().gravity.y, -300.0f, 1e-4);
    CHECK(loaded.GetPhysicsWorld().settings.substeps == 8);
    CHECK(!loaded.GetPhysicsWorld().settings.enableSleep);
    CHECK(loaded.GetPhysicsWorld().getBodyCount() == 5);

    auto find = [&](const char *name) -> AccelEngine::RigidBody *
    {
        for (auto handle : loaded.GetRegistry().view<TagComponent>())
            if (loaded.GetRegistry().get<TagComponent>(handle).Tag == name)
                if (auto *physics = loaded.GetRegistry().try_get<PhysicsComponent>(handle))
                    return physics->body.get();
        return nullptr;
    };

    AccelEngine::RigidBody *c = find("Circle");
    CHECK(c != nullptr);
    if (c)
    {
        CHECK(c->shapeType == AccelEngine::ShapeType::CIRCLE);
        CHECK_NEAR(c->circle.radius, 12.0f, 1e-4);
        CHECK_NEAR(c->getMass(), 3.0f, 1e-3);
        CHECK_NEAR(c->restitution, 0.7f, 1e-4);
        CHECK(c->categoryBits == 0x0004 && c->maskBits == 0x00F1 && c->groupIndex == -2);
        CHECK(c->isSensor && c->ignoreGravity && !c->canSleep);
        CHECK_NEAR(c->linearDamping, 0.8f, 1e-4);
        CHECK_NEAR(c->orientation, 0.5f, 1e-4);
        CHECK_NEAR(c->position.x, 70.0f, 1e-3);
        CHECK_NEAR(c->position.y, -20.0f, 1e-3);
        CHECK_NEAR(c->rotation, 1.25f, 1e-4);
        CHECK_NEAR(c->velocity.y, -5.0f, 1e-4);
    }
    AccelEngine::RigidBody *k = find("Capsule");
    CHECK(k != nullptr && k->shapeType == AccelEngine::ShapeType::CAPSULE && k->parts.size() == 1);
    if (k)
    {
        CHECK(k->oneWay);
        CHECK_NEAR(k->oneWayNormal.x, 1.0f, 1e-4);
        CHECK_NEAR(k->parts[0].radius, 9.0f, 1e-4);
    }
    AccelEngine::RigidBody *h = find("Hexagon");
    CHECK(h != nullptr && h->shapeType == AccelEngine::ShapeType::POLYGON && h->parts[0].count == 6);
    AccelEngine::RigidBody *m = find("Compound");
    CHECK(m != nullptr && m->shapeType == AccelEngine::ShapeType::COMPOUND && m->parts.size() == 2);
    AccelEngine::RigidBody *b = find("Box");
    CHECK(b != nullptr && b->shapeType == AccelEngine::ShapeType::AABB);
    if (b)
        CHECK_NEAR(b->aabb.halfSize.x, 30.0f, 1e-4);
}

TEST_CASE("physics: scenes saved by older versions still load")
{
    const std::string yaml =
        "Scene: Old\nVersion: 1\nEntities:\n"
        "  - ID: 1\n    Tag: Crate\n    Transform: {position: [50, 60], rotation: 0, scale: [1, 1]}\n"
        "    Physics:\n      shape: AABB\n      position: [50, 60]\n      rotation: 2.0\n      inverse_mass: 0.5\n"
        "      half_size: [10, 12]\n      velocity: [3, 4]\n      restitution: 0.4\n      static_friction: 0.5\n"
        "      dynamic_friction: 0.3\n      lock_rotation: false\n      lock_position: false\n";
    Scene scene("Old");
    CHECK(SceneSerializer(&scene).DeserializeFromString(yaml));
    CHECK(scene.GetPhysicsWorld().getBodyCount() == 1);
    CHECK_NEAR(scene.GetPhysicsWorld().gravity.y, -980.0f, 1e-4);
    for (auto handle : scene.GetRegistry().view<PhysicsComponent>())
    {
        const auto &body = *scene.GetRegistry().get<PhysicsComponent>(handle).body;
        CHECK_NEAR(body.getMass(), 2.0f, 1e-4);
        CHECK_NEAR(body.rotation, 2.0f, 1e-4);
        CHECK_NEAR(body.velocity.x, 3.0f, 1e-4);
        CHECK_NEAR(body.aabb.halfSize.y, 12.0f, 1e-4);
    }
}

TEST_CASE("physics: Lua can create bodies, query the world and make joints")
{
    GameVariables::Clear();
    const fs::path root = fs::temp_directory_path() / "simp-physics-lua";
    std::error_code ec;
    fs::remove_all(root, ec);
    fs::create_directories(root / "scripts", ec);
    {
        std::ofstream file(root / "scripts" / "body.lua");
        file << R"(
local S = {}
function S:OnCreate()
    Physics.SetGravity(0, -500)
    local gx, gy = Physics.GetGravity()
    Game.Set("lua_gravity", gy)

    local body = self.entity:Add("Body")
    body:CreateBox(40, 20, 2)
    Game.Set("lua_shape", body:GetShape() == "box" and 1 or 0)
    Game.Set("lua_mass", body:GetMass())
    local w, h = body:GetSize()
    Game.Set("lua_width", w)

    body:SetLayer(3)
    body:SetCollidesWith(0, 1)
    Game.Set("lua_layer", body:GetLayer())
    Game.Set("lua_mask", body:GetMask())
    body:SetSensor(true)
    Game.Set("lua_sensor", body:IsSensor() and 1 or 0)
    body:SetSensor(false)

    local wall = Scene.Find("Wall")
    local hit, hx, hy, nx, ny, dist = Physics.Raycast(50, 0, 1, 0, 1000)
    Game.Set("lua_ray", (hit and hit:GetName() == "Wall") and 1 or 0)
    Game.Set("lua_ray_distance", dist or -1)
    Game.Set("lua_miss", Physics.Raycast(50, 0, 0, 1, 1000) == nil and 1 or 0)
    Game.Set("lua_point", Physics.QueryPoint(0, 0) ~= nil and 1 or 0)
    Game.Set("lua_box", #Physics.QueryBox(-500, -500, 500, 500))
    Game.Set("lua_circle", #Physics.QueryCircle(300, 0, 5))
    Game.Set("lua_bodies", Physics.GetBodyCount())

    local joint = Physics.CreateWeldJoint(self.entity, wall)
    Game.Set("lua_joint", joint:GetType() == "weld" and 1 or 0)
    joint:Destroy()
    Game.Set("lua_joint_gone", joint:IsValid() and 0 or 1)
    local distance = Physics.CreateDistanceJoint(self.entity, wall, 0, 0, 0, 0, 120)
    Game.Set("lua_length", distance:GetLength())
    local ok = pcall(function() distance:SetMotor(1, 1) end)
    Game.Set("lua_motor_error", ok and 0 or 1)
end
return S
)";
    }

    VFS::Mount("assets", root.string());
    {
        Scene scene("Lua");
        Entity wall = scene.CreateEntity("Wall");
        wall.GetComponent<TransformComponent>().position = {300.0f, 0.0f};
        PhysicsBodies::Options options;
        options.Mass = 0.0f;
        scene.AddBody(wall.GetHandle(), PhysicsBodies::Box({20.0f, 50.0f}, options));

        Entity actor = scene.CreateEntity("Actor");
        actor.AddComponent<PhysicsComponent>();
        actor.AddComponent<ScriptComponent>().BindLua("assets://scripts/body.lua");
        Run(scene, 3);
    }
    VFS::UnMount("assets");

    CHECK_NEAR(GameVariables::Get("lua_gravity"), -500.0, 1e-3);
    CHECK(GameVariables::Get("lua_shape") == 1.0);
    CHECK_NEAR(GameVariables::Get("lua_mass"), 2.0, 1e-4);
    CHECK_NEAR(GameVariables::Get("lua_width"), 40.0, 1e-3);
    CHECK(GameVariables::Get("lua_layer") == 3.0);
    CHECK(GameVariables::Get("lua_mask") == 3.0);
    CHECK(GameVariables::Get("lua_sensor") == 1.0);
    CHECK(GameVariables::Get("lua_ray") == 1.0);
    CHECK_NEAR(GameVariables::Get("lua_ray_distance"), 230.0, 1.0);
    CHECK(GameVariables::Get("lua_miss") == 1.0);
    CHECK(GameVariables::Get("lua_point") == 1.0);
    CHECK(GameVariables::Get("lua_box") == 2.0);
    CHECK(GameVariables::Get("lua_circle") == 1.0);
    CHECK(GameVariables::Get("lua_bodies") == 2.0);
    CHECK(GameVariables::Get("lua_joint") == 1.0);
    CHECK(GameVariables::Get("lua_joint_gone") == 1.0);
    CHECK_NEAR(GameVariables::Get("lua_length"), 120.0, 1e-3);
    CHECK(GameVariables::Get("lua_motor_error") == 1.0);
    GameVariables::Clear();
    fs::remove_all(root, ec);
}
