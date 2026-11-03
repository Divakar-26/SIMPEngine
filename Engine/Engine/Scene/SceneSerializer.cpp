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
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

namespace SIMPEngine {
namespace {
void EmitColor(YAML::Emitter &out, const SDL_Color &c) {
  out << YAML::Flow << YAML::BeginSeq << static_cast<int>(c.r)
      << static_cast<int>(c.g) << static_cast<int>(c.b)
      << static_cast<int>(c.a) << YAML::EndSeq;
}

bool ReadColor(const YAML::Node &node, SDL_Color &c) {
  if (!node || !node.IsSequence() || node.size() < 4)
    return false;
  c.r = static_cast<Uint8>(std::clamp(node[0].as<int>(), 0, 255));
  c.g = static_cast<Uint8>(std::clamp(node[1].as<int>(), 0, 255));
  c.b = static_cast<Uint8>(std::clamp(node[2].as<int>(), 0, 255));
  c.a = static_cast<Uint8>(std::clamp(node[3].as<int>(), 0, 255));
  return true;
}

bool ReadVec2(const YAML::Node &node, float &x, float &y) {
  if (!node || !node.IsSequence() || node.size() < 2)
    return false;
  x = node[0].as<float>();
  y = node[1].as<float>();
  return true;
}

std::string ResolvePath(const std::string &filepath) {
  if (auto resolved = VFS::Resolve(filepath))
    return *resolved;
  CORE_WARN("Failed to resolve VFS path '{}'", filepath);
  return filepath;
}
} // namespace

std::string SceneSerializer::Emit(const std::vector<entt::entity> &entities,
                                  bool includeSettings) {
  auto &registry = m_Scene->GetRegistry();

  std::unordered_map<entt::entity, std::uint64_t> entityIds;
  std::uint64_t nextEntityId = 1;
  for (auto entityHandle : entities)
    entityIds.emplace(entityHandle, nextEntityId++);

  YAML::Emitter out;
  out << YAML::BeginMap;

  out << YAML::Key << "Scene" << YAML::Value << m_Scene->GetName();

  if (includeSettings) {
    const glm::vec4 &bg = m_Scene->GetBackgroundColor();
    out << YAML::Key << "Settings" << YAML::BeginMap;
    out << YAML::Key << "background" << YAML::Value << YAML::Flow
        << YAML::BeginSeq << bg.r << bg.g << bg.b << bg.a << YAML::EndSeq;
    out << YAML::EndMap;
  }

  out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

  for (auto entityHandle : entities) {
    if (!registry.valid(entityHandle) ||
        !registry.all_of<TagComponent>(entityHandle))
      continue;

    Entity entity{entityHandle, m_Scene};
    auto &tag = entity.GetComponent<TagComponent>();

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
      out << YAML::Key << "color" << YAML::Value;
      EmitColor(out, rc.color);
      out << YAML::Key << "size" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << rc.width << rc.height << YAML::EndSeq;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<SpriteComponent>()) {
      auto &sc = entity.GetComponent<SpriteComponent>();
      out << YAML::Key << "Sprite" << YAML::BeginMap;
      out << YAML::Key << "size" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << sc.width << sc.height << YAML::EndSeq;
      if (sc.texture) {
        out << YAML::Key << "texture" << YAML::Value
            << sc.texture->GetVFSPath();
        out << YAML::Key << "filter" << YAML::Value
            << (sc.texture->GetFilter() == TextureFilter::Nearest ? "nearest"
                                                                  : "linear");
      }
      out << YAML::Key << "tint" << YAML::Value;
      EmitColor(out, sc.tint);
      out << YAML::Key << "flip" << YAML::Value << YAML::Flow
          << YAML::BeginSeq << sc.flipX << sc.flipY << YAML::EndSeq;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<TextComponent>()) {
      auto &tx = entity.GetComponent<TextComponent>();
      out << YAML::Key << "Text" << YAML::BeginMap;
      out << YAML::Key << "text" << YAML::Value << tx.Text;
      out << YAML::Key << "size" << YAML::Value << tx.Size;
      out << YAML::Key << "color" << YAML::Value;
      EmitColor(out, tx.Color);
      out << YAML::Key << "align" << YAML::Value << tx.Align;
      if (!tx.FontPath.empty())
        out << YAML::Key << "font" << YAML::Value << tx.FontPath;
      out << YAML::EndMap;
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
            << static_cast<float>(anim->GetFrameDuration());
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
        const auto chunkY = static_cast<std::int32_t>(static_cast<std::uint32_t>(packed));

        out << YAML::BeginMap;
        out << YAML::Key << "x" << YAML::Value << chunkX;
        out << YAML::Key << "y" << YAML::Value << chunkY;
        out << YAML::Key << "tiles" << YAML::BeginSeq;
        for (int localY = 0; localY < CHUNK_SIZE; ++localY) {
          for (int localX = 0; localX < CHUNK_SIZE; ++localX) {
            const int tileId = chunk.tiles[static_cast<size_t>(localY * CHUNK_SIZE + localX)].id;
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
  }

  out << YAML::EndSeq;
  out << YAML::EndMap;

  if (!out.good()) {
    CORE_ERROR("Scene serialization failed: {}", out.GetLastError());
    return {};
  }

  return out.c_str();
}

std::string SceneSerializer::SerializeToString() {
  std::vector<entt::entity> entities;
  for (auto entityHandle : m_Scene->GetRegistry().view<TagComponent>())
    entities.push_back(entityHandle);
  std::reverse(entities.begin(), entities.end());
  return Emit(entities, true);
}

std::string
SceneSerializer::SerializeEntities(const std::vector<entt::entity> &entities) {
  return Emit(entities, false);
}

bool SceneSerializer::Serialize(const std::string &filepath) {
  std::string yaml = SerializeToString();
  if (yaml.empty())
    return false;

  std::string realPath = ResolvePath(filepath);
  std::ofstream fout(realPath);
  if (!fout.is_open()) {
    CORE_ERROR("Could not open '{}' for writing", realPath);
    return false;
  }
  fout << yaml;

  return true;
}

bool SceneSerializer::Deserialize(const std::string &filepath) {
  std::string realPath = ResolvePath(filepath);

  std::ifstream stream(realPath);
  if (!stream.is_open())
    return false;

  std::stringstream buffer;
  buffer << stream.rdbuf();
  return DeserializeFromString(buffer.str());
}

bool SceneSerializer::DeserializeFromString(const std::string &yaml,
                                            std::vector<entt::entity> *created) {
  YAML::Node data;
  try {
    data = YAML::Load(yaml);
  } catch (const YAML::Exception &e) {
    CORE_ERROR("Failed to parse scene: {}", e.what());
    return false;
  }
  return Load(data, true, created);
}

std::vector<entt::entity>
SceneSerializer::DeserializeEntities(const std::string &yaml) {
  std::vector<entt::entity> created;
  try {
    Load(YAML::Load(yaml), false, &created);
  } catch (const YAML::Exception &e) {
    CORE_ERROR("Failed to parse entities: {}", e.what());
  }
  return created;
}

bool SceneSerializer::Load(const YAML::Node &data, bool applySettings,
                           std::vector<entt::entity> *created) {
  if (!data["Scene"])
    return false;

  if (applySettings) {
    std::string sceneName;
    if (TryGet(data, "Scene", sceneName))
      m_Scene->SetName(sceneName);

    if (auto settings = data["Settings"]) {
      auto bg = settings["background"];
      if (bg && bg.IsSequence() && bg.size() >= 4)
        m_Scene->SetBackgroundColor({bg[0].as<float>(), bg[1].as<float>(),
                                     bg[2].as<float>(), bg[3].as<float>()});
    }
  }

  std::unordered_map<std::uint64_t, entt::entity> entityIds;
  std::vector<std::pair<std::uint64_t, YAML::Node>> serializedEntities;
  std::uint64_t nextFallbackId = 1;

  auto entities = data["Entities"];
  if (!entities)
    return true;

  for (auto entityNode : entities) {
    std::string tag;
    try {
      if (!TryGet(entityNode, "Tag", tag))
        continue;
    } catch (const YAML::Exception &e) {
      CORE_ERROR("Entity deserialize error: {}", e.what());
      continue;
    }

    std::uint64_t serializedId = nextFallbackId++;
    if (entityNode["ID"])
      serializedId = entityNode["ID"].as<std::uint64_t>();

    Entity entity = m_Scene->CreateEntity(tag);
    entityIds[serializedId] = entity.GetHandle();
    serializedEntities.emplace_back(serializedId, entityNode);
    if (created)
      created->push_back(entity.GetHandle());

    auto component = [&](const char *name, auto &&load) {
      auto node = entityNode[name];
      if (!node)
        return;
      try {
        load(node);
      } catch (const YAML::Exception &e) {
        CORE_ERROR("'{}': failed to load {}: {}", tag, name, e.what());
      }
    };

    component("Transform", [&](const YAML::Node &t) {
      auto &tc = entity.GetComponent<TransformComponent>();
      ReadVec2(t["position"], tc.position.x, tc.position.y);
      if (t["rotation"])
        TryGet(t, "rotation", tc.rotation);
      if (t["zIndex"])
        TryGet(t, "zIndex", tc.zIndex);
      ReadVec2(t["scale"], tc.scale.x, tc.scale.y);
    });

    component("Sprite", [&](const YAML::Node &s) {
      auto &sc = entity.AddComponent<SpriteComponent>();
      ReadVec2(s["size"], sc.width, sc.height);
      if (s["texture"]) {
        std::string texPath = s["texture"].as<std::string>();
        if (!texPath.empty())
          sc.texture = TextureManager::Get().LoadTexture(texPath, texPath);
        if (sc.texture && s["filter"])
          sc.texture->SetFilter(s["filter"].as<std::string>() == "linear"
                                    ? TextureFilter::Linear
                                    : TextureFilter::Nearest);
      }
      ReadColor(s["tint"], sc.tint);
      if (auto flip = s["flip"]; flip && flip.IsSequence() && flip.size() >= 2) {
        sc.flipX = flip[0].as<bool>();
        sc.flipY = flip[1].as<bool>();
      }
    });

    component("Text", [&](const YAML::Node &t) {
      auto &tx = entity.AddComponent<TextComponent>();
      if (t["text"])
        tx.Text = t["text"].as<std::string>();
      if (t["size"])
        tx.Size = t["size"].as<float>();
      ReadColor(t["color"], tx.Color);
      if (t["align"])
        tx.Align = t["align"].as<int>();
      if (t["font"])
        tx.FontPath = t["font"].as<std::string>();
    });

    component("Render", [&](const YAML::Node &r) {
      auto &rc = entity.AddComponent<RenderComponent>();
      ReadColor(r["color"], rc.color);
      ReadVec2(r["size"], rc.width, rc.height);
    });

    component("Collision", [&](const YAML::Node &c) {
      auto &cc = entity.AddComponent<CollisionComponent>();
      ReadVec2(c["size"], cc.width, cc.height);
      ReadVec2(c["offset"], cc.offsetX, cc.offsetY);
    });

    component("Velocity", [&](const YAML::Node &v) {
      auto &vc = entity.AddComponent<VelocityComponent>();
      ReadVec2(v["value"], vc.vx, vc.vy);
    });

    component("Camera", [&](const YAML::Node &cam) {
      float zoom = 1.0f;
      if (cam["zoom"])
        TryGet(cam, "zoom", zoom);

      auto &cc = entity.AddComponent<CameraComponent>(zoom);
      if (cam["primary"])
        TryGet(cam, "primary", cc.primary);
    });

    component("Lifetime", [&](const YAML::Node &l) {
      auto &lf = entity.AddComponent<LifetimeComponent>();
      if (l["remaining"])
        TryGet(l, "remaining", lf.remaining);
    });

    component("Animation", [&](const YAML::Node &a) {
      std::string path;
      int fw = 0, fh = 0, fn = 0;
      float fd = 0.0f;
      bool loop = false;

      if (!TryGet(a, "texture", path) || !TryGet(a, "frame_width", fw) ||
          !TryGet(a, "frame_height", fh) || !TryGet(a, "frame_count", fn) ||
          !TryGet(a, "frame_duration", fd) || !TryGet(a, "looping", loop))
        return;

      auto tex = TextureManager::Get().LoadTexture(path, path);
      if (!tex)
        return;

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
    });

    component("Tilemap", [&](const YAML::Node &tilemapNode) {
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
    });

    component("Physics", [&](const YAML::Node &p) {
      std::string shape;
      if (!TryGet(p, "shape", shape))
        return;

      float px = 0.0f, py = 0.0f;
      if (!ReadVec2(p["position"], px, py))
        return;

      float invMass = 0.0f;
      if (!TryGet(p, "inverse_mass", invMass))
        return;

      auto body = std::make_unique<AccelEngine::RigidBody>();
      body->entityID = static_cast<uint32_t>(entity.GetHandle());
      body->position = {px, py};
      body->inverseMass = invMass;

      if (shape == "CIRCLE") {
        float radius = 0.0f;
        if (!TryGet(p, "radius", radius))
          return;
        body->shapeType = AccelEngine::ShapeType::CIRCLE;
        body->circle.radius = radius;
      } else {
        float hx = 0.0f, hy = 0.0f;
        if (!ReadVec2(p["half_size"], hx, hy))
          return;
        body->shapeType = AccelEngine::ShapeType::AABB;
        body->aabb.halfSize = {hx, hy};
      }

      TryGet(p, "rotation", body->rotation);

      float vx = 0.0f, vy = 0.0f;
      if (ReadVec2(p["velocity"], vx, vy))
        body->velocity = {vx, vy};

      TryGet(p, "restitution", body->restitution);
      TryGet(p, "static_friction", body->staticFriction);
      TryGet(p, "dynamic_friction", body->dynamicFriction);
      TryGet(p, "lock_rotation", body->lockRotation);
      TryGet(p, "lock_position", body->lockPosition);

      body->calculateInertia();

      auto &phys = entity.AddComponent<PhysicsComponent>();
      phys.body = std::move(body);
      m_Scene->GetPhysicsWorld().addBody(phys.body.get());
      m_Scene->GetPhysicsSystem().bodies.push_back(phys.body.get());
    });
  }

  for (const auto &[childId, entityNode] : serializedEntities) {
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

  m_Scene->UpdateTransforms();
  return true;
}
} // namespace SIMPEngine
