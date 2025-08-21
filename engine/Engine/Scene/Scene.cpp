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

        entity.AddComponent<TransformComponent>(0.0f, 0.0f, 0.0f);

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

        if (useMainCamera)
        {
            m_MainCamera.Update(deltaTime);
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
                    camComp.Camera.Update(deltaTime);
                    break;
                }
            }
        }

        // For testing: move all entities right slowly
        // auto view = m_Registry.view<TransformComponent>();
        // for (auto entity : view)
        // {
        //     auto &transform = view.get<TransformComponent>(entity);
        //     transform.x += 50.0f * deltaTime; // moves 50 units/sec
        // }

        auto view = m_Registry.view<TransformComponent, VelocityComponent>();
        for (auto entity : view)
        {
            auto &transform = view.get<TransformComponent>(entity);
            auto &velocity = view.get<VelocityComponent>(entity);

            transform.x += velocity.vx * deltaTime;
            transform.y += velocity.vy * deltaTime;
        }

        // Example simple collision check between entities with CollisionComponent
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
                }
            }
        }
    }

    void Scene::OnRender()
    {
        Camera2D *activeCamera = nullptr;

        if (useMainCamera)
        {
            activeCamera = &m_MainCamera;
        }
        else
        {
            auto cameraView = m_Registry.view<CameraComponent>();
            for (auto entity : cameraView)
            {
                auto &camComp = cameraView.get<CameraComponent>(entity);
                if (camComp.primary)
                {
                    activeCamera = &camComp.Camera;
                    break;
                }
            }
        }

        if (activeCamera)
        {
            Renderer::SetViewMatrix(activeCamera->GetViewMatrix());
        }

        // auto collidable = m_Registry.view<TransformComponent, CollisionComponent>();
        // for (auto entity : collidable)
        // {
        //     auto &transform = collidable.get<TransformComponent>(entity);
        //     auto &collision = collidable.get<CollisionComponent>(entity);

        //     SDL_FRect rect = collision.GetBoundsWorld(transform);

        //     // Optional: offset to camera if needed
        //     // rect.x -= cameraX;
        //     // rect.y -= cameraY;

        //     // Draw outline
        //     Renderer::DrawQuad(rect.x - 2.0f, rect.y - 2.0f, rect.w * transform.scaleX + 4.0f, rect.h * transform.scaleY + 4.0f, SDL_Color{255, 255, 0, 255});
        // }

        auto spriteView = m_Registry.view<TransformComponent, SpriteComponent>();
        for (auto entity : spriteView)
        {
            auto &transform = spriteView.get<TransformComponent>(entity);
            auto &spriteComp = spriteView.get<SpriteComponent>(entity);

            Renderer::DrawTexture(spriteComp.texture->GetSDLTexture(), transform.x, transform.y, spriteComp.width, spriteComp.height, SDL_Color{255, 255, 255, 255}, transform.rotation);
        }

        // auto view = m_Registry.view<TransformComponent, RenderComponent>();
        // for (auto entity : view)
        // {
        //     auto &transform = view.get<TransformComponent>(entity);
        //     auto &render = view.get<RenderComponent>(entity);
        //     Renderer::DrawQuad(transform.x, transform.y, render.width * transform.scaleX, render.height * transform.scaleY, render.color);
        // }
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
        auto view = m_Registry.view<CameraComponent>();
        for (auto entity : view)
        {
            auto &camComp = view.get<CameraComponent>(entity);
            if (camComp.primary)
            {
                return camComp.Camera;
            }
        }

        return m_MainCamera;
    }

}
