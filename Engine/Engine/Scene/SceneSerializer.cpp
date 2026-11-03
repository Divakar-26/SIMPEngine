#include "yaml-cpp/emittermanip.h"
#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Rendering/TextureManager.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/SceneSerializer.h>
#include <Engine/Scene/TilemapUtils.h>

#include <cstdint>
#include <fstream>
#include <unordered_map>
#include <utility>
#include <vector>

namespace SIMPEngine {
bool SceneSerializer::Serialize(const std::string &filepath) {
  std::string realPath = filepath;
  if (auto resolved = VFS::Resolve(filepath))
    realPath = *resolved;
  else
    CORE_WARN("Failed to resolve VFS path '{}'", filepath);

  std::unordered_map<entt::entity, std::uint64_t> entityIds;
  std::uint64_t nextEntityId = 1;
  auto entityView = m_Scene->GetRegistry().view<TagComponent>();
  for (auto entityHandle : entityView)
    entityIds.emplace(entityHandle, nextEntityId++);

  YAML::Emitter out;
  out << YAML::BeginMap;

  out << YAML::Key << "Scene" << YAML::Value << m_Scene->GetName();
  out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

  entityView.each([&](auto entityHandle, auto &tag) {
    Entity entity{entityHandle, m_Scene};

    out << YAML::BeginMap;

    out << YAML::Key << "ID" << YAML::Value << entityIds.at(entityHandle);

    out << YAML::Key << "Tag" << YAML::Value << tag.Tag;

    if (entity.HasComponent<TransformComponent>()) {
      auto &tc = entity.GetComponent<TransformComponent>();

      out << YAML::Key << "Transform" << YAML::BeginMap;
      out << YAML::Key << "position" << YAML::Value << YAML::Flow
          << YAML::BeginSeq << tc.position.x << tc.position.y << YAML::EndSeq;
      out << YAML::Key << "rotation" << YAML::Value << tc.rotation;
      out << YAML::Key << "zIndex" << YAML::Value << tc.zIndex;
      out << YAML::Key << "scale" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << tc.scale.x << tc.scale.y << YAML::EndSeq;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<RenderComponent>()) {
      auto &rc = entity.GetComponent<RenderComponent>();

      out << YAML::Key << "Render" << YAML::BeginMap;
      out << YAML::Key << "color" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << (int)rc.color.r << (int)rc.color.g << (int)rc.color.b
          << (int)rc.color.a << YAML::EndSeq;
      out << YAML::Key << "size" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << rc.width << rc.height << YAML::EndSeq;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<SpriteComponent>()) {
      auto &sc = entity.GetComponent<SpriteComponent>();
      if (sc.texture) {
        out << YAML::Key << "Sprite" << YAML::BeginMap;
        out << YAML::Key << "size" << YAML::Value << YAML::Flow
            << YAML::BeginSeq << sc.width << sc.height << YAML::EndSeq;
        out << YAML::Key << "texture" << YAML::Value
            << sc.texture->GetVFSPath();
        out << YAML::EndMap;
      }
    }

    if (entity.HasComponent<AnimatedSpriteComponent>()) {
      auto &ac = entity.GetComponent<AnimatedSpriteComponent>();
      auto *anim = ac.animation.get();

      if (!anim || !anim->GetTexture()) {
        CORE_WARN("Skipping animation serialization for '{}' due to missing animation/texture", tag.Tag);
      } else {
        out << YAML::Key << "Animation" << YAML::BeginMap;
        out << YAML::Key << "texture" << YAML::Value
            << anim->GetTexture()->GetVFSPath();
        out << YAML::Key << "frame_width" << YAML::Value << anim->GetFrameWidth();
        out << YAML::Key << "frame_height" << YAML::Value
            << anim->GetFrameHeight();
        out << YAML::Key << "frame_count" << YAML::Value
            << anim->GetFrameNumber();
        out << YAML::Key << "frame_duration" << YAML::Value
            << (float)anim->GetFrameDuration();
        out << YAML::Key << "looping" << YAML::Value << anim->IsLooping();
        out << YAML::Key << "current_frame" << YAML::Value
            << anim->GetCurrentFrame();
        out << YAML::Key << "elapsed_time" << YAML::Value
            << anim->GetElapsedTime();
        out << YAML::Key << "finished" << YAML::Value
            << anim->IsFinished();
        out << YAML::EndMap;
      }
    }

    if (entity.HasComponent<CollisionComponent>()) {
      auto &cc = entity.GetComponent<CollisionComponent>();

      out << YAML::Key << "Collision" << YAML::BeginMap;
      out << YAML::Key << "size" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << cc.width << cc.height << YAML::EndSeq;
      out << YAML::Key << "offset" << YAML::Value << YAML::Flow
          << YAML::BeginSeq << cc.offsetX << cc.offsetY << YAML::EndSeq;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<VelocityComponent>()) {
      auto &vc = entity.GetComponent<VelocityComponent>();

      out << YAML::Key << "Velocity" << YAML::BeginMap;
      out << YAML::Key << "value" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << vc.vx << vc.vy << YAML::EndSeq;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<CameraComponent>()) {
      auto &cam = entity.GetComponent<CameraComponent>();

      out << YAML::Key << "Camera" << YAML::BeginMap;
      out << YAML::Key << "primary" << YAML::Value << cam.primary;
      out << YAML::Key << "zoom" << YAML::Value << cam.Camera.GetZoom();
      out << YAML::EndMap;
    }

    if (entity.HasComponent<HierarchyComponent>()) {
      const auto &hierarchy = entity.GetComponent<HierarchyComponent>();
      std::uint64_t parentId = 0;
      if (hierarchy.parent != entt::null) {
        auto parent = entityIds.find(hierarchy.parent);
        if (parent != entityIds.end())
          parentId = parent->second;
      }

      out << YAML::Key << "Hierarchy" << YAML::BeginMap;
      out << YAML::Key << "parent_id" << YAML::Value << parentId;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<TilemapComponent>()) {
      const auto &tilemap = entity.GetComponent<TilemapComponent>();
      out << YAML::Key << "Tilemap" << YAML::BeginMap;
      out << YAML::Key << "tile_size" << YAML::Value << tilemap.tileSize;

      if (tilemap.tileset && tilemap.tileset->GetTexture()) {
        out << YAML::Key << "texture" << YAML::Value
            << tilemap.tileset->GetTexture()->GetVFSPath();
        out << YAML::Key << "tile_width" << YAML::Value
            << tilemap.tileset->GetTileWidth();
        out << YAML::Key << "tile_height" << YAML::Value
            << tilemap.tileset->GetTileHeight();
      }

      out << YAML::Key << "chunks" << YAML::Value << YAML::BeginSeq;
      for (const auto &[key, chunk] : tilemap.chunks) {
        const auto packed = static_cast<std::uint64_t>(key);
        const auto chunkX = static_cast<std::int32_t>(packed >> 32);
        const auto chunkY = static_cast<std::int32_t>(packed & 0xffffffffu);

        out << YAML::BeginMap;
        out << YAML::Key << "x" << YAML::Value << chunkX;
        out << YAML::Key << "y" << YAML::Value << chunkY;
        out << YAML::Key << "tiles" << YAML::BeginSeq;
        for (int localY = 0; localY < CHUNK_SIZE; ++localY) {
          for (int localX = 0; localX < CHUNK_SIZE; ++localX) {
            const int tileId = chunk.tiles[localY * CHUNK_SIZE + localX].id;
            if (tileId >= 0)
              out << YAML::Flow << YAML::BeginSeq << localX << localY << tileId
                  << YAML::EndSeq;
          }
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;
      }
      out << YAML::EndSeq;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<LifetimeComponent>()) {
      auto &lf = entity.GetComponent<LifetimeComponent>();
      out << YAML::Key << "Lifetime" << YAML::BeginMap;
      out << YAML::Key << "remaining" << YAML::Value << lf.remaining;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<PhysicsComponent>()) {
      auto &phys = entity.GetComponent<PhysicsComponent>();
      auto *body = phys.body.get();

      if (!body) {
        CORE_WARN("Skipping physics serialization for '{}' due to missing body", tag.Tag);
      } else {
        out << YAML::Key << "Physics" << YAML::BeginMap;

      std::string shape = (body->shapeType == AccelEngine::ShapeType::CIRCLE)
                              ? "CIRCLE"
                              : "AABB";

      out << YAML::Key << "shape" << YAML::Value << shape;

      out << YAML::Key << "position" << YAML::Value << YAML::Flow
          << YAML::BeginSeq << body->position.x << body->position.y
          << YAML::EndSeq;

      out << YAML::Key << "rotation" << YAML::Value << body->rotation;

      out << YAML::Key << "inverse_mass" << YAML::Value << body->inverseMass;

      if (shape == "CIRCLE") {
        out << YAML::Key << "radius" << YAML::Value << body->circle.radius;
      } else {
        out << YAML::Key << "half_size" << YAML::Value << YAML::Flow
            << YAML::BeginSeq << body->aabb.halfSize.x << body->aabb.halfSize.y
            << YAML::EndSeq;
      }

      out << YAML::Key << "velocity" << YAML::Value << YAML::Flow
          << YAML::BeginSeq << body->velocity.x << body->velocity.y
          << YAML::EndSeq;

      out << YAML::Key << "restitution" << YAML::Value << body->restitution;

      out << YAML::Key << "static_friction" << YAML::Value
          << body->staticFriction;

      out << YAML::Key << "dynamic_friction" << YAML::Value
          << body->dynamicFriction;

      out << YAML::Key << "lock_rotation" << YAML::Value << body->lockRotation;

      out << YAML::Key << "lock_position" << YAML::Value << body->lockPosition;

      out << YAML::EndMap;
      }
    }

    out << YAML::EndMap;
  });

  out << YAML::EndSeq;
  out << YAML::EndMap;

  if (!out.good()) {
    CORE_ERROR("Scene serialization failed: {}", out.GetLastError());
    return false;
  }

  std::ofstream fout(realPath);
  if (!fout.is_open()) {
    CORE_ERROR("Could not open '{}' for writing", realPath);
    return false;
  }
  fout << out.c_str();

  return true;
}

bool SceneSerializer::Deserialize(const std::string &filepath) {
  std::string realPath = filepath;
  if (auto resolved = VFS::Resolve(filepath))
    realPath = *resolved;
  else
    CORE_WARN("Failed to resolve VFS path '{}'", filepath);

  std::ifstream stream(realPath);
  if (!stream.is_open())
    return false;

  YAML::Node data;
  try {
    data = YAML::Load(stream);
  } catch (const YAML::Exception &e) {
    CORE_ERROR("Failed to parse scene '{}': {}", realPath, e.what());
    return false;
  }
  if (!data["Scene"])
    return false;

  std::string sceneName;
  if (TryGet(data, "Scene", sceneName))
    m_Scene->SetName(sceneName);

  std::unordered_map<std::uint64_t, entt::entity> entityIds;
  std::vector<std::pair<std::uint64_t, YAML::Node>> serializedEntities;
  std::uint64_t nextFallbackId = 1;

  auto entities = data["Entities"];
  if (!entities)
    return true;

  for (auto entityNode : entities) {
    try {
      std::string tag;
      if (!TryGet(entityNode, "Tag", tag))
        continue;

      std::uint64_t serializedId = nextFallbackId++;
      if (entityNode["ID"])
        serializedId = entityNode["ID"].as<std::uint64_t>();

      Entity entity = m_Scene->CreateEntity(tag);
      entityIds[serializedId] = entity.GetHandle();
      serializedEntities.emplace_back(serializedId, entityNode);

      if (auto t = entityNode["Transform"]) {
        auto &tc = entity.GetComponent<TransformComponent>();

        auto pos = t["position"];
        auto scale = t["scale"];

        if (pos && pos.size() >= 2) {
          tc.position.x = pos[0].as<float>();
          tc.position.y = pos[1].as<float>();
        }

        TryGet(t, "rotation", tc.rotation);
        if (t["zIndex"])
          TryGet(t, "zIndex", tc.zIndex);

        if (scale && scale.size() >= 2) {
          tc.scale.x = scale[0].as<float>();
          tc.scale.y = scale[1].as<float>();
        }
      }

      if (auto s = entityNode["Sprite"]) {
        auto size = s["size"];
        std::string texPath;

        if (!size || size.size() < 2)
          continue;

        if (!TryGet(s, "texture", texPath))
          continue;

        auto &sc = entity.AddComponent<SpriteComponent>();
        sc.width = size[0].as<float>();
        sc.height = size[1].as<float>();
        sc.texture = TextureManager::Get().LoadTexture(texPath, texPath);
      }

      if (auto r = entityNode["Render"]) {
        auto color = r["color"];
        auto size = r["size"];

        if (!color || color.size() < 4 || !size || size.size() < 2)
          continue;

        auto &rc = entity.AddComponent<RenderComponent>();
        rc.color.r = color[0].as<Uint8>();
        rc.color.g = color[1].as<Uint8>();
        rc.color.b = color[2].as<Uint8>();
        rc.color.a = color[3].as<Uint8>();
        rc.width = size[0].as<float>();
        rc.height = size[1].as<float>();
      }

      if (auto c = entityNode["Collision"]) {
        auto size = c["size"];
        auto offset = c["offset"];

        if (!size || size.size() < 2 || !offset || offset.size() < 2)
          continue;

        auto &cc = entity.AddComponent<CollisionComponent>();
        cc.width = size[0].as<float>();
        cc.height = size[1].as<float>();
        cc.offsetX = offset[0].as<float>();
        cc.offsetY = offset[1].as<float>();
      }

      if (auto v = entityNode["Velocity"]) {
        auto val = v["value"];
        if (!val || val.size() < 2)
          continue;

        auto &vc = entity.AddComponent<VelocityComponent>();
        vc.vx = val[0].as<float>();
        vc.vy = val[1].as<float>();
      }

      if (auto cam = entityNode["Camera"]) {
        bool primary;
        if (!TryGet(cam, "primary", primary))
          continue;

        float zoom = 1.0f;
        if (cam["zoom"])
          TryGet(cam, "zoom", zoom);

        auto &cc = entity.AddComponent<CameraComponent>(zoom);
        cc.primary = primary;
      }

      if (auto l = entityNode["Lifetime"]) {
        float remaining;
        if (TryGet(l, "remaining", remaining)) {
          auto &lf = entity.AddComponent<LifetimeComponent>();
          lf.remaining = remaining;
        }
      }

      if (auto a = entityNode["Animation"]) {
        std::string path;
        int fw, fh, fn;
        float fd;
        bool loop;

        if (!TryGet(a, "texture", path) || !TryGet(a, "frame_width", fw) ||
            !TryGet(a, "frame_height", fh) || !TryGet(a, "frame_count", fn) ||
            !TryGet(a, "frame_duration", fd) || !TryGet(a, "looping", loop))
          continue;

        auto tex = TextureManager::Get().LoadTexture(path, path);
        if (!tex)
          continue;

        auto &ac = entity.AddComponent<AnimatedSpriteComponent>();
        ac.animation = std::make_unique<Animation>(tex, fw, fh, fn, fd, loop);
        int currentFrame = 0;
        float elapsedTime = 0.0f;
        bool finished = false;
        if (a["current_frame"])
          TryGet(a, "current_frame", currentFrame);
        if (a["elapsed_time"])
          TryGet(a, "elapsed_time", elapsedTime);
        if (a["finished"])
          TryGet(a, "finished", finished);
        ac.animation->SetPlaybackState(currentFrame, elapsedTime, finished);
      }

      if (auto tilemapNode = entityNode["Tilemap"]) {
        auto &tilemap = entity.AddComponent<TilemapComponent>();
        if (tilemapNode["tile_size"])
          TryGet(tilemapNode, "tile_size", tilemap.tileSize);

        std::string texturePath;
        int tileWidth = 0;
        int tileHeight = 0;
        const bool hasTileset = tilemapNode["texture"] &&
                                tilemapNode["tile_width"] &&
                                tilemapNode["tile_height"];
        if (hasTileset) {
          TryGet(tilemapNode, "texture", texturePath);
          TryGet(tilemapNode, "tile_width", tileWidth);
          TryGet(tilemapNode, "tile_height", tileHeight);
        }
        if (hasTileset && tileWidth > 0 && tileHeight > 0) {
          auto texture = TextureManager::Get().LoadTexture(texturePath, texturePath);
          if (texture)
            tilemap.tileset = std::make_shared<Tileset>(texture, tileWidth, tileHeight);
        }

        if (auto chunks = tilemapNode["chunks"]; chunks && chunks.IsSequence()) {
          for (const auto &chunkNode : chunks) {
            int chunkX = 0;
            int chunkY = 0;
            if (!TryGet(chunkNode, "x", chunkX) || !TryGet(chunkNode, "y", chunkY))
              continue;
            auto tiles = chunkNode["tiles"];
            if (!tiles || !tiles.IsSequence())
              continue;
            for (const auto &tileNode : tiles) {
              if (!tileNode.IsSequence() || tileNode.size() < 3)
                continue;
              const int localX = tileNode[0].as<int>();
              const int localY = tileNode[1].as<int>();
              const int tileId = tileNode[2].as<int>();
              if (localX < 0 || localX >= CHUNK_SIZE || localY < 0 ||
                  localY >= CHUNK_SIZE || tileId < 0)
                continue;
              SetTile(tilemap, chunkX * CHUNK_SIZE + localX,
                      chunkY * CHUNK_SIZE + localY, tileId);
            }
          }
        }
      }

      if (auto p = entityNode["Physics"]) {
        std::string shape;
        if (!TryGet(p, "shape", shape))
          continue;

        auto pos = p["position"];
        if (!pos || pos.size() < 2)
          continue;

        float invMass;
        if (!TryGet(p, "inverse_mass", invMass))
          continue;

        auto &phys = entity.AddComponent<PhysicsComponent>();
        phys.body = std::make_unique<AccelEngine::RigidBody>();

        phys.body->entityID = (uint32_t)entity.GetHandle();

        phys.body->position = {pos[0].as<float>(), pos[1].as<float>()};

        phys.body->inverseMass = invMass;

        if (shape == "CIRCLE") {
          float radius;
          if (!TryGet(p, "radius", radius))
            continue;

          phys.body->shapeType = AccelEngine::ShapeType::CIRCLE;
          phys.body->circle.radius = radius;
        } else {
          auto hs = p["half_size"];
          if (!hs || hs.size() < 2)
            continue;

          phys.body->shapeType = AccelEngine::ShapeType::AABB;
          phys.body->aabb.halfSize = {hs[0].as<float>(), hs[1].as<float>()};
        }

        TryGet(p, "rotation", phys.body->rotation);

        auto vel = p["velocity"];
        if (vel && vel.size() >= 2) {
          phys.body->velocity = {vel[0].as<float>(), vel[1].as<float>()};
        }

        TryGet(p, "restitution", phys.body->restitution);
        TryGet(p, "static_friction", phys.body->staticFriction);
        TryGet(p, "dynamic_friction", phys.body->dynamicFriction);
        TryGet(p, "lock_rotation", phys.body->lockRotation);
        TryGet(p, "lock_position", phys.body->lockPosition);

        phys.body->calculateInertia();

        m_Scene->GetPhysicsWorld().addBody(phys.body.get());
        m_Scene->GetPhysicsSystem().bodies.push_back(phys.body.get());
      }
    } catch (const YAML::Exception &e) {
      CORE_ERROR("Entity deserialize error: {}", e.what());
    }
  }

  for (const auto &[childId, entityNode] : serializedEntities) {
    (void)childId;
    auto hierarchyNode = entityNode["Hierarchy"];
    if (!hierarchyNode || !hierarchyNode["parent_id"])
      continue;

    std::uint64_t parentId = 0;
    if (!TryGet(hierarchyNode, "parent_id", parentId) || parentId == 0)
      continue;

    auto childIt = entityIds.find(childId);
    auto parentIt = entityIds.find(parentId);
    if (childIt == entityIds.end() || parentIt == entityIds.end() ||
        childIt->second == parentIt->second)
      continue;

    m_Scene->SetParent(childIt->second, parentIt->second);
  }

  return true;
}
}
