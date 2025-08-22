#include "Scene.h"
#include "Entity.h"
#include "../Rendering/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "Log.h"
#include "Input/Input.h"
#include <entt/entt.hpp>
#include <iostream>

namespace SIMPEngine
{
    Scene::Scene(const std::string &name)
        : m_Name(name)
    {
    }

    void Scene::DestroyEntity(Entity entity)
    {
        m_Registry.destroy(entity.GetHandle());
    }

    Entity Scene::CreatePlayer()
    {
        // Entity player = CreateEntity("Player");

        // auto &transform = player.AddComponent<TransformComponent>();
        // transform.x = 100.0f;
        // transform.y = 200.0f;

        // auto &sprite = player.AddComponent<SpriteComponent>();
        // // sprite.texture = TextureManager::Get().LoadTexture("coin", "../assets/coin.png", Renderer::GetSDLRenderer());
        // sprite.width = 100.0f;
        // sprite.height = 120.0f;

        // return player;
    }

    Entity Scene::CreateEntity(const std::string &name)
    {
        entt::entity handle = m_Registry.create();
        Entity entity(handle, this);

        entity.AddComponent<TransformComponent>(100.0f, 0.0f, 0.0f);
        entity.AddComponent<TagComponent>(name.empty() ? "Unnamed Entity" : name);
        return entity;
    }

    Entity Scene::GetEntityByName(const std::string &name)
    {
        auto view = m_Registry.view<TagComponent>();
        for (auto e : view)
        {
            auto &tag = view.get<TagComponent>(e);
            if (tag.Tag == name)
                return Entity(e, this);
        }
        return Entity{};
    }

    void Scene::OnUpdate(float deltaTime)
    {
        static bool cWasDown = false;
        bool cIsDown = Input::IsKeyPressed(SDLK_C);

        if (cIsDown && !cWasDown)
        {
            useMainCamera = !useMainCamera;
            CORE_INFO("Camera toggled. Now using: {}", useMainCamera ? "MainCamera" : "EntityCamera");
        }
        cWasDown = cIsDown;

        UpdateCamera(deltaTime);
        UpdateEntities(deltaTime);
        CheckCollision(deltaTime);
    }

    void Scene::OnRender()
    {
        Camera2D *activeCamera = GetActiveCameraPtr();

        if (activeCamera)
        {
            Renderer::SetViewMatrix(activeCamera->GetViewMatrix());
        }

        RenderSprites();
        RenderQuad();
    }

    bool Scene::HasActiveCamera()
    {
        for (auto entity : m_Registry.view<CameraComponent, TransformComponent>())
        {
            auto &cameraComp = m_Registry.get<CameraComponent>(entity);
            if (cameraComp.primary)
                return true;
        }
        return false;
    }

    Camera2D &Scene::GetActiveCamera()
    {
        if (useMainCamera)
            return m_MainCamera;
        auto view = m_Registry.view<CameraComponent>();
        for (auto entity : view)
        {
            auto &camComp = view.get<CameraComponent>(entity);
            if (camComp.primary)
            {
                return camComp.Camera;
            }
        }
    }

    void Scene::UpdateCamera(float dt)
    {

        if (useMainCamera)
        {
            m_MainCamera.Update(dt);
        }
        else
        {
            auto cameraView = m_Registry.view<CameraComponent, TransformComponent>();
            for (auto entity : cameraView)
            {
                auto &camComp = cameraView.get<CameraComponent>(entity);
                auto &transform = cameraView.get<TransformComponent>(entity);

                if (camComp.primary)
                {
                    camComp.Camera.SetPosition({transform.x, transform.y});
                    camComp.Camera.Update(dt);
                    break;
                }
            }
        }
    }
    void Scene::UpdateEntities(float dt)
    {
        auto view = m_Registry.view<TransformComponent, VelocityComponent>();
        for (auto entity : view)
        {
            auto &transform = view.get<TransformComponent>(entity);
            auto &velocity = view.get<VelocityComponent>(entity);

            transform.x += velocity.vx * dt;
            transform.y += velocity.vy * dt;

            CORE_ERROR("HELLO FOUND");
        }
    }
    void Scene::CheckCollision(float dt)
    {
        auto collidable = m_Registry.view<TransformComponent, CollisionComponent>();
        for (auto entityA : collidable)
        {
            auto &aTransform = collidable.get<TransformComponent>(entityA);
            auto &aCollision = collidable.get<CollisionComponent>(entityA);
            SDL_FRect aRect = aCollision.GetBounds(aTransform.x, aTransform.y);

            for (auto entityB : collidable)
            {
                if (entityA == entityB)
                    continue;

                auto &bTransform = collidable.get<TransformComponent>(entityB);
                auto &bCollision = collidable.get<CollisionComponent>(entityB);
                SDL_FRect bRect = bCollision.GetBounds(bTransform.x, bTransform.y);

                if (SDL_HasRectIntersectionFloat(&aRect, &bRect))
                {
                    // CORE_INFO("Collision detected between entities!");
                    CORE_ERROR("COLLISION");
                }
            }
        }
    }

    void Scene::RenderSprites()
    {
        auto spriteView = m_Registry.view<TransformComponent, SpriteComponent>();
        for (auto entity : spriteView)
        {
            auto &transform = spriteView.get<TransformComponent>(entity);
            auto &spriteComp = spriteView.get<SpriteComponent>(entity);

            if (spriteComp.texture)
                Renderer::DrawTexture(spriteComp.texture->GetSDLTexture(), transform.x, transform.y, spriteComp.width, spriteComp.height, SDL_Color{255, 255, 255, 255}, transform.rotation);
        }
    }
    void Scene::RenderQuad()
    {
        auto view = m_Registry.view<TransformComponent, RenderComponent>();
        for (auto entity : view)
        {
            auto &transform = view.get<TransformComponent>(entity);
            auto &render = view.get<RenderComponent>(entity);
            Renderer::DrawQuad(transform.x, transform.y, render.width * transform.scaleX, render.height * transform.scaleY, render.color);
        }
    }

    Camera2D *Scene::GetActiveCameraPtr()
    {
        if (useMainCamera)
            return &m_MainCamera;

        auto view = m_Registry.view<CameraComponent>();
        for (auto entity : view)
        {
            auto &camComp = view.get<CameraComponent>(entity);
            if (camComp.primary)
                return &camComp.Camera;
        }
        return nullptr;
    }
}
