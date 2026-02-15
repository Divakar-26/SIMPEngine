#include <Engine/Core/Log.h>
#include <Engine/Input/Input.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Rendering/TextureManager.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/ScriptableEntity.h>

#include <entt/entt.hpp>
#include <iostream>

namespace SIMPEngine {
Scene::Scene(const std::string &name) : m_Name(name) {
  physicsSystem.SetSubstep(20);
}

void Scene::DestroyEntity(Entity entity) {
  m_Registry.destroy(entity.GetHandle());
}

Entity Scene::CreatePlayer() {
  // Entity player = CreateEntity("Player");

  // auto &transform = player.AddComponent<TransformComponent>();
  // transform.x = 100.0f;
  // transform.y = 200.0f;
  // auto &sprite = player.AddComponent<SpriteComponent>();
  // // sprite.texture = TextureManager::Get().LoadTexture("coin",
  // "../assets/coin.png", Renderer::GetSDLRenderer()); sprite.width = 100.0f;
  // sprite.height = 120.0f;

  // return player;
}

Entity Scene::CreateEntity(const std::string &name) {
  entt::entity handle = m_Registry.create();
  Entity entity(handle, this);

  entity.AddComponent<TransformComponent>();
  entity.AddComponent<TagComponent>(name.empty() ? "Unnamed Entity" : name);
  return entity;
}

Entity Scene::GetEntityByName(const std::string &name) {
  auto view = m_Registry.view<TagComponent>();
  for (auto e : view) {
    auto &tag = view.get<TagComponent>(e);
    if (tag.Tag == name)
      return Entity(e, this);
  }
  return Entity{};
}

void Scene::OnUpdate(float deltaTime) {
  // start phycsis frame
  physicsSystem.startFrame(deltaTime);

  // update transfrom
  physicsSystem.UpdateTransform(m_Registry);

  cameraSystem.OnUpdate(m_Registry, deltaTime);
  movementSystem.Update(m_Registry, deltaTime);
  collisionSystem.Update(m_Registry, deltaTime);
  hierarchySystem.Update(m_Registry);
  lifetimeSystem.Update(m_Registry, deltaTime);

  auto view = m_Registry.view<ScriptComponent>();
  for (auto entityHandle : view) {
    auto &sc = view.get<ScriptComponent>(entityHandle);

    if (!sc.InstantiateScript)
      continue;

    if (!sc.Instance) {
      sc.Instance = sc.InstantiateScript();
      sc.Instance->m_Entity = Entity(entityHandle, this);
      sc.Instance->OnCreate();
    }

    sc.Instance->OnUpdate(deltaTime);
  }

  // update physics
  physicsSystem.Update(m_Registry, deltaTime);
  // collision checking

  // upgrading all animation
  auto animView = m_Registry.view<AnimatedSpriteComponent>();
  for (auto entity : animView) {
    auto &anim = animView.get<AnimatedSpriteComponent>(entity);
    if (anim.animation)
      anim.animation->Update(deltaTime);
  }
}

// TODO -> Z INDEX SYSTEM -> DONE
void Scene::OnRender() {
  cameraSystem.OnRender(m_Registry);
  RenderQuad();
  RenderSprites();
  RenderColliders();
}

void Scene::RenderSprites() {
  std::vector<std::pair<entt::entity, float>> entitiesWithZIndex;

  // Collect all entities that have a Sprite or Animation
  auto view = m_Registry.view<TransformComponent>();

  for (auto entity : view) {
    // Must have Transform + (Sprite OR Animation)
    if (m_Registry.any_of<SpriteComponent>(entity) ||
        m_Registry.any_of<AnimatedSpriteComponent>(entity)) {
      auto &transform = m_Registry.get<TransformComponent>(entity);
      entitiesWithZIndex.emplace_back(entity, transform.zIndex);
    }
  }

  // Sort by zIndex (lower first → back to front)
  std::sort(entitiesWithZIndex.begin(), entitiesWithZIndex.end(),
            [](const auto &a, const auto &b) { return a.second < b.second; });

  // Render in correct order
  for (const auto &[entity, zIndex] : entitiesWithZIndex) {
    auto &transform = m_Registry.get<TransformComponent>(entity);
    float width = 1.0f;
    float height = 1.0f;

    if (m_Registry.any_of<RenderComponent>(entity)) {
      auto &render = m_Registry.get<RenderComponent>(entity);
      width = render.width * transform.scale.x;
      height = render.height * transform.scale.y;
    }

    if (m_Registry.any_of<AnimatedSpriteComponent>(entity)) {
      auto &anim = m_Registry.get<AnimatedSpriteComponent>(entity);
      if (anim.animation) {
        anim.animation->Draw(transform.position.x, transform.position.y, width,
                             height, SDL_Color{255, 255, 255, 255},
                             transform.rotation);

        continue;
      }
    }

    if (m_Registry.any_of<SpriteComponent>(entity)) {
      auto &spriteComp = m_Registry.get<SpriteComponent>(entity);
      if (spriteComp.texture) {
        Renderer::DrawTexture(spriteComp.texture, transform.position.x,
                              transform.position.y, width, height,
                              SDL_Color{255, 255, 255, 255}, transform.rotation,
                              zIndex);
      }
    }
  }
}

void Scene::RenderQuad() {
  std::vector<std::pair<entt::entity, float>> entitiesWithZIndex;
  auto view = m_Registry.view<TransformComponent, RenderComponent>();

  for (auto entity : view) {
    if (m_Registry.any_of<SpriteComponent>(entity) ||
        m_Registry.any_of<AnimatedSpriteComponent>(entity)) {
      // auto &sprite = m_Registry.get<SpriteComponent>(entity);
      // if (sprite.texture && sprite.texture->GetID() != 0)
      continue;
    }

    auto &transform = view.get<TransformComponent>(entity);
    entitiesWithZIndex.emplace_back(entity, transform.zIndex);
  }

  std::sort(entitiesWithZIndex.begin(), entitiesWithZIndex.end(),
            [](const auto &a, const auto &b) { return a.second < b.second; });

  for (const auto &[entity, zIndex] : entitiesWithZIndex) {
    auto &transform = m_Registry.get<TransformComponent>(entity);
    auto &render = m_Registry.get<RenderComponent>(entity);

    glm::vec3 worldPos = transform.worldTransform[3];

    Renderer::DrawQuad(worldPos.x, worldPos.y, render.width * transform.scale.x,
                       render.height * transform.scale.y, transform.rotation,
                       render.color, true, zIndex);
  }
}

void Scene::RenderColliders() {
  std::vector<std::pair<entt::entity, float>> entitiesWithZIndex;

  auto view = m_Registry.view<TransformComponent, CollisionComponent>();
  for (auto entity : view) {
    auto &transform = view.get<TransformComponent>(entity);
    entitiesWithZIndex.emplace_back(entity, transform.zIndex);
  }

  std::sort(entitiesWithZIndex.begin(), entitiesWithZIndex.end(),
            [](const auto &a, const auto &b) { return a.second < b.second; });

  for (const auto &[entity, zIndex] : entitiesWithZIndex) {
    auto &transform = m_Registry.get<TransformComponent>(entity);
    auto &collider = m_Registry.get<CollisionComponent>(entity);

    SDL_FRect bounds = collider.GetBoundsWorld(transform);

    SDL_Color fillColor = {200, 85, 90, 200};
    SDL_Color outlineColor = {68, 85, 90, 200};

    Renderer::DrawQuad(bounds.x, bounds.y, bounds.w, bounds.h,
                       transform.rotation, fillColor, true, zIndex + 1);

    Renderer::DrawQuad(bounds.x - 2, bounds.y - 2, bounds.w + 4, bounds.h + 4,
                       transform.rotation, outlineColor, false, zIndex + 1);
  }
}

void Scene::Clear() {
  // Destroy physics bodies
  physicsSystem.DestroyBodies();
  physicsSystem.DestroyWorld();

  // Destroy all ECS entities
  m_Registry.clear();
}

} // namespace SIMPEngine
