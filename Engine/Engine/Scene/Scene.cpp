#include "Scene.h"
#include "Entity.h"
#include "../Rendering/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "Core/Log.h"
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
    }

    void Scene::OnRender()
    {

        // call camera render
        cameraSystem.OnRender(m_Registry);
        RenderSprites();
        RenderQuad();
    }

    void Scene::RenderSprites()
    {
        auto spriteView = m_Registry.view<TransformComponent, SpriteComponent>();
        for (auto entity : spriteView)
        {
            auto &transform = spriteView.get<TransformComponent>(entity);
            auto &spriteComp = spriteView.get<SpriteComponent>(entity);

            if (spriteComp.texture)
                Renderer::DrawTexture(spriteComp.texture->GetSDLTexture(), transform.position.x, transform.position.y, spriteComp.width, spriteComp.height, SDL_Color{255, 255, 255, 255}, transform.rotation);
        }
    }

    void Scene::RenderQuad()
    {
        auto view = m_Registry.view<TransformComponent, RenderComponent>();
        for (auto entity : view)
        {
            auto &transform = view.get<TransformComponent>(entity);
            auto &render = view.get<RenderComponent>(entity);
            Renderer::DrawQuad(transform.position.x, transform.position.y, render.width * transform.scale.x, render.height * transform.scale.y, render.color);
        }
    }

}
