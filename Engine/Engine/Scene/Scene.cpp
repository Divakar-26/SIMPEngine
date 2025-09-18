#include "Scene.h"
#include "Entity.h"
#include "../Rendering/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "Core/Log.h"
#include "Input/Input.h"
#include "Scene/ScriptableEntity.h"
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

        entity.AddComponent<TransformComponent>();
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

        cameraSystem.OnUpdate(m_Registry, deltaTime);

        movementSystem.Update(m_Registry, deltaTime);
        collisionSystem.Update(m_Registry, deltaTime);

        auto view = m_Registry.view<ScriptComponent>();
        for (auto entityHandle : view)
        {
            auto &sc = view.get<ScriptComponent>(entityHandle);

            if (!sc.Instance)
            {
                sc.Instance = sc.InstantiateScript();
                sc.Instance->m_Entity = Entity(entityHandle, this);
                sc.Instance->OnCreate();
            }

            sc.Instance->OnUpdate(deltaTime);
        }
    }

    // TODO -> Z INDEX SYSTEM
    void Scene::OnRender()
    {

        cameraSystem.OnRender(m_Registry);
        RenderQuad();
        RenderSprites();
        RenderColliders();
    }

    void Scene::RenderSprites()
    {
        auto spriteView = m_Registry.view<TransformComponent, SpriteComponent>();
        for (auto entity : spriteView)
        {
            auto &transform = spriteView.get<TransformComponent>(entity);
            auto &spriteComp = spriteView.get<SpriteComponent>(entity);

            if (spriteComp.texture)
            {
                Renderer::DrawTexture(
                    spriteComp.texture->GetID(),
                    transform.position.x,
                    transform.position.y,
                    spriteComp.width * transform.scale.x,
                    spriteComp.height * transform.scale.y,
                    SDL_Color{255, 255, 255, 255},
                    transform.rotation);
            }
        }
    }

    void Scene::RenderQuad()
    {
        auto view = m_Registry.view<TransformComponent, RenderComponent>();

        for (auto entity : view)
        {
            if (m_Registry.any_of<SpriteComponent>(entity))
            {
                auto &sprite = m_Registry.get<SpriteComponent>(entity);
                if (sprite.texture && sprite.texture->GetID() != 0)
                    continue;
            }

            auto &transform = view.get<TransformComponent>(entity);
            auto &render = view.get<RenderComponent>(entity);

            Renderer::DrawQuad(
                transform.position.x,
                transform.position.y,
                render.width * transform.scale.x,
                render.height * transform.scale.y,
                render.color);
        }
    }

    void Scene::RenderColliders()
    {
        auto view = m_Registry.view<TransformComponent, CollisionComponent>();
        for (auto entity : view)
        {
            auto &transform = view.get<TransformComponent>(entity);
            auto &collider = view.get<CollisionComponent>(entity);

            float x = transform.position.x;
            float y = transform.position.y;
            float w = collider.width * transform.scale.x;
            float h = collider.height * transform.scale.y;

            SDL_Color fillColor = {0, 0, 255, 100};
            Renderer::DrawQuad(x, y, w, h, fillColor, true);

            SDL_Color outlineColor = {173, 216, 230, 255};
            Renderer::DrawQuad(x, y, w, h, outlineColor, false);
        }
    }

}
