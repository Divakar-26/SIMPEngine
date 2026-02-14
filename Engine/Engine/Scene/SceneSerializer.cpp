#include "yaml-cpp/emittermanip.h"
#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Rendering/TextureManager.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/SceneSerializer.h>

#include <fstream>

namespace SIMPEngine {

bool SceneSerializer::Serialize(const std::string &filepath) {
  std::string realPath = filepath;
  if (auto resolved = VFS::Resolve(filepath))
    realPath = *resolved;
  else
    CORE_WARN("Failed to resolve VFS path '{}'", filepath);

  YAML::Emitter out;
  out << YAML::BeginMap;

  out << YAML::Key << "Scene" << YAML::Value << m_Scene->GetName();
  out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

  m_Scene->GetRegistry().view<TagComponent>().each([&](auto entityHandle,
                                                       auto &tag) {
    Entity entity{entityHandle, m_Scene};

    out << YAML::BeginMap;

    // Tag
    out << YAML::Key << "Tag" << YAML::Value << tag.Tag;

    // Transform
    if (entity.HasComponent<TransformComponent>()) {
      auto &tc = entity.GetComponent<TransformComponent>();

      out << YAML::Key << "Transform" << YAML::BeginMap;
      out << YAML::Key << "position" << YAML::Value << YAML::Flow
          << YAML::BeginSeq << tc.position.x << tc.position.y << YAML::EndSeq;
      out << YAML::Key << "rotation" << YAML::Value << tc.rotation;
      out << YAML::Key << "scale" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << tc.scale.x << tc.scale.y << YAML::EndSeq;
      out << YAML::EndMap;
    }

    // Render
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

    // Sprite
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

    // Animation
    if (entity.HasComponent<AnimatedSpriteComponent>()) {
      auto &ac = entity.GetComponent<AnimatedSpriteComponent>();
      auto *anim = ac.animation;

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
      out << YAML::EndMap;
    }

    // Collision
    if (entity.HasComponent<CollisionComponent>()) {
      auto &cc = entity.GetComponent<CollisionComponent>();

      out << YAML::Key << "Collision" << YAML::BeginMap;
      out << YAML::Key << "size" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << cc.width << cc.height << YAML::EndSeq;
      out << YAML::Key << "offset" << YAML::Value << YAML::Flow
          << YAML::BeginSeq << cc.offsetX << cc.offsetY << YAML::EndSeq;
      out << YAML::EndMap;
    }

    // Velocity
    if (entity.HasComponent<VelocityComponent>()) {
      auto &vc = entity.GetComponent<VelocityComponent>();

      out << YAML::Key << "Velocity" << YAML::BeginMap;
      out << YAML::Key << "value" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << vc.vx << vc.vy << YAML::EndSeq;
      out << YAML::EndMap;
    }

    // Camera
    if (entity.HasComponent<CameraComponent>()) {
      auto &cam = entity.GetComponent<CameraComponent>();

      out << YAML::Key << "Camera" << YAML::BeginMap;
      out << YAML::Key << "primary" << YAML::Value << cam.primary;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<LifetimeComponent>()) {
      auto &lf = entity.GetComponent<LifetimeComponent>();
      out << YAML::Key << "Lifetime" << YAML::BeginMap;
      out << YAML::Key << "remaining" << YAML::Value << lf.remaining;
      out << YAML::EndMap;
    }

    // Physics
    if (entity.HasComponent<PhysicsComponent>()) {
      auto &phys = entity.GetComponent<PhysicsComponent>();
      auto *body = phys.body;

      out << YAML::Key << "Physics" << YAML::BeginMap;

      // Shape
      std::string shape = (body->shapeType == AccelEngine::ShapeType::CIRCLE)
                              ? "CIRCLE"
                              : "AABB";

      out << YAML::Key << "shape" << YAML::Value << shape;

      // Transform-like data
      out << YAML::Key << "position" << YAML::Value << YAML::Flow
          << YAML::BeginSeq << body->position.x << body->position.y
          << YAML::EndSeq;

      out << YAML::Key << "rotation" << YAML::Value << body->rotation;

      out << YAML::Key << "inverse_mass" << YAML::Value << body->inverseMass;

      // Shape specific
      if (shape == "CIRCLE") {
        out << YAML::Key << "radius" << YAML::Value << body->circle.radius;
      } else {
        out << YAML::Key << "half_size" << YAML::Value << YAML::Flow
            << YAML::BeginSeq << body->aabb.halfSize.x << body->aabb.halfSize.y
            << YAML::EndSeq;
      }

      // Motion
      out << YAML::Key << "velocity" << YAML::Value << YAML::Flow
          << YAML::BeginSeq << body->velocity.x << body->velocity.y
          << YAML::EndSeq;

      // Material
      out << YAML::Key << "restitution" << YAML::Value << body->restitution;

      out << YAML::Key << "static_friction" << YAML::Value
          << body->staticFriction;

      out << YAML::Key << "dynamic_friction" << YAML::Value
          << body->dynamicFriction;

      // Constraints
      out << YAML::Key << "lock_rotation" << YAML::Value << body->lockRotation;

      out << YAML::Key << "lock_position" << YAML::Value << body->lockPosition;

      out << YAML::EndMap;
    }

    out << YAML::EndMap;
  });

  out << YAML::EndSeq;
  out << YAML::EndMap;

  std::ofstream fout(realPath);
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

  YAML::Node data = YAML::Load(stream);
  if (!data["Scene"])
    return false;

  std::string sceneName;
  if (TryGet(data, "Scene", sceneName))
    m_Scene->SetName(sceneName);

  auto entities = data["Entities"];
  if (!entities)
    return true;

  for (auto entityNode : entities) {
    try {
      std::string tag;
      if (!TryGet(entityNode, "Tag", tag))
        continue;

      Entity entity = m_Scene->CreateEntity(tag);

      if (auto t = entityNode["Transform"]) {
        auto &tc = entity.GetComponent<TransformComponent>();

        auto pos = t["position"];
        auto scale = t["scale"];

        if (pos && pos.size() >= 2) {
          tc.position.x = pos[0].as<float>();
          tc.position.y = pos[1].as<float>();
        }

        TryGet(t, "rotation", tc.rotation);

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

        auto &cc = entity.AddComponent<CameraComponent>();
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
        ac.animation = new Animation(tex, fw, fh, fn, fd, loop);
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
        phys.body = new AccelEngine::RigidBody();

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

        m_Scene->physicsWorld.addBody(phys.body);
        m_Scene->bodies.push_back(phys.body);
      }
    } catch (const YAML::Exception &e) {
      CORE_ERROR("Entity deserialize error: {}", e.what());
    }
  }

  return true;
}

} // namespace SIMPEngine
