#include <Engine/Core/FileSystem.h>
#include "yaml-cpp/emittermanip.h"
#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Rendering/TextureManager.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/SceneSerializer.h>
#include <Engine/Scene/TilemapUtils.h>
#include <Engine/Particles/ParticleSerializer.h>

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

void EmitPath(YAML::Emitter &out, const VectorPath &path) {
  out << YAML::BeginSeq;
  for (const VectorContour &contour : path.Contours) {
    out << YAML::BeginMap;
    out << YAML::Key << "closed" << YAML::Value << contour.Closed;
    out << YAML::Key << "nodes" << YAML::Value << YAML::BeginSeq;
    for (const VectorNode &n : contour.Nodes) {
      out << YAML::Flow << YAML::BeginSeq << n.Position.x << n.Position.y;
      if (n.In != glm::vec2(0.0f) || n.Out != glm::vec2(0.0f))
        out << n.In.x << n.In.y << n.Out.x << n.Out.y;
      out << YAML::EndSeq;
    }
    out << YAML::EndSeq;
    out << YAML::EndMap;
  }
  out << YAML::EndSeq;
}

bool ReadPath(const YAML::Node &node, VectorPath &path) {
  if (!node || !node.IsSequence())
    return false;
  path.Clear();
  for (const YAML::Node &c : node) {
    VectorContour contour;
    if (c["closed"])
      contour.Closed = c["closed"].as<bool>();
    if (c["nodes"] && c["nodes"].IsSequence()) {
      for (const YAML::Node &n : c["nodes"]) {
        if (!n.IsSequence() || n.size() < 2)
          continue;
        VectorNode vn;
        vn.Position = {n[0].as<float>(), n[1].as<float>()};
        if (n.size() >= 6) {
          vn.In = {n[2].as<float>(), n[3].as<float>()};
          vn.Out = {n[4].as<float>(), n[5].as<float>()};
        }
        contour.Nodes.push_back(vn);
      }
    }
    path.Contours.push_back(std::move(contour));
  }
  return true;
}

const char *CapName(StrokeCap cap) {
  return cap == StrokeCap::Butt ? "butt" : cap == StrokeCap::Square ? "square" : "round";
}

const char *JoinName(StrokeJoin join) {
  return join == StrokeJoin::Miter ? "miter" : join == StrokeJoin::Bevel ? "bevel" : "round";
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
  out << YAML::Key << "Version" << YAML::Value << FormatVersion;

  if (includeSettings) {
    const glm::vec4 &bg = m_Scene->GetBackgroundColor();
    out << YAML::Key << "Settings" << YAML::BeginMap;
    out << YAML::Key << "background" << YAML::Value << YAML::Flow
        << YAML::BeginSeq << bg.r << bg.g << bg.b << bg.a << YAML::EndSeq;
    const SceneLighting &lighting = m_Scene->GetLighting();
    out << YAML::Key << "lighting" << YAML::BeginMap;
    out << YAML::Key << "enabled" << YAML::Value << lighting.Enabled;
    out << YAML::Key << "ambient" << YAML::Value << YAML::Flow << YAML::BeginSeq
        << lighting.Ambient.r << lighting.Ambient.g << lighting.Ambient.b
        << lighting.Ambient.a << YAML::EndSeq;
    out << YAML::EndMap;
    const BloomSettings &bloom = m_Scene->GetBloom();
    out << YAML::Key << "bloom" << YAML::BeginMap;
    out << YAML::Key << "enabled" << YAML::Value << bloom.Enabled;
    out << YAML::Key << "threshold" << YAML::Value << bloom.Threshold;
    out << YAML::Key << "knee" << YAML::Value << bloom.Knee;
    out << YAML::Key << "intensity" << YAML::Value << bloom.Intensity;
    out << YAML::Key << "radius" << YAML::Value << bloom.Radius;
    out << YAML::EndMap;
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
      if (sc.useRegion)
        out << YAML::Key << "region" << YAML::Value << YAML::Flow
            << YAML::BeginSeq << sc.region.x << sc.region.y << sc.region.w
            << sc.region.h << YAML::EndSeq;
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
        if (!ac.playing)
          out << YAML::Key << "playing" << YAML::Value << false;
        if (ac.speed != 1.0f)
          out << YAML::Key << "speed" << YAML::Value << ac.speed;
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
      if (!cam.followTarget.empty()) {
        out << YAML::Key << "follow" << YAML::Value << cam.followTarget;
        out << YAML::Key << "follow_offset" << YAML::Value << YAML::Flow
            << YAML::BeginSeq << cam.followOffset.x << cam.followOffset.y
            << YAML::EndSeq;
        out << YAML::Key << "smoothing" << YAML::Value << cam.followSmoothing;
        out << YAML::Key << "dead_zone" << YAML::Value << YAML::Flow
            << YAML::BeginSeq << cam.deadZone.x << cam.deadZone.y
            << YAML::EndSeq;
      }
      if (cam.useBounds) {
        out << YAML::Key << "bounds" << YAML::Value << YAML::Flow
            << YAML::BeginSeq << cam.boundsMin.x << cam.boundsMin.y
            << cam.boundsMax.x << cam.boundsMax.y << YAML::EndSeq;
      }
      out << YAML::Key << "shake_strength" << YAML::Value << cam.shakeStrength;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<LightComponent>()) {
      const auto &light = entity.GetComponent<LightComponent>();
      out << YAML::Key << "Light" << YAML::BeginMap;
      out << YAML::Key << "type" << YAML::Value
          << (light.type == LightType::Spot ? "spot" : "point");
      out << YAML::Key << "color" << YAML::Value;
      EmitColor(out, light.color);
      out << YAML::Key << "intensity" << YAML::Value << light.intensity;
      out << YAML::Key << "radius" << YAML::Value << light.radius;
      out << YAML::Key << "falloff" << YAML::Value << light.falloff;
      out << YAML::Key << "inner_angle" << YAML::Value << light.innerAngle;
      out << YAML::Key << "outer_angle" << YAML::Value << light.outerAngle;
      out << YAML::Key << "flicker" << YAML::Value << light.flicker;
      out << YAML::Key << "flicker_speed" << YAML::Value << light.flickerSpeed;
      out << YAML::Key << "enabled" << YAML::Value << light.enabled;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<GlowComponent>()) {
      const auto &glow = entity.GetComponent<GlowComponent>();
      out << YAML::Key << "Glow" << YAML::BeginMap;
      out << YAML::Key << "color" << YAML::Value;
      EmitColor(out, glow.color);
      out << YAML::Key << "intensity" << YAML::Value << glow.intensity;
      out << YAML::Key << "spread" << YAML::Value << glow.spread;
      out << YAML::Key << "enabled" << YAML::Value << glow.enabled;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<VectorShapeComponent>()) {
      const auto &v = entity.GetComponent<VectorShapeComponent>();
      out << YAML::Key << "VectorShape" << YAML::BeginMap;
      out << YAML::Key << "path" << YAML::Value;
      EmitPath(out, v.path);
      if (!v.morphTarget.Empty()) {
        out << YAML::Key << "morph_target" << YAML::Value;
        EmitPath(out, v.morphTarget);
        out << YAML::Key << "morph" << YAML::Value << v.morph;
      }
      out << YAML::Key << "fill" << YAML::Value << v.fill;
      out << YAML::Key << "fill_color" << YAML::Value;
      EmitColor(out, v.fillColor);
      out << YAML::Key << "stroke" << YAML::Value << v.stroke;
      out << YAML::Key << "stroke_color" << YAML::Value;
      EmitColor(out, v.strokeColor);
      out << YAML::Key << "stroke_width" << YAML::Value << v.strokeWidth;
      out << YAML::Key << "cap" << YAML::Value << CapName(v.cap);
      out << YAML::Key << "join" << YAML::Value << JoinName(v.join);
      out << YAML::Key << "miter_limit" << YAML::Value << v.miterLimit;
      out << YAML::Key << "trim" << YAML::Value << YAML::Flow << YAML::BeginSeq << v.trimStart << v.trimEnd << v.trimOffset << YAML::EndSeq;
      if (v.dashLength > 0.0f)
        out << YAML::Key << "dash" << YAML::Value << YAML::Flow << YAML::BeginSeq << v.dashLength << v.dashGap << v.dashOffset << YAML::EndSeq;
      if (v.gradient) {
        out << YAML::Key << "gradient" << YAML::BeginMap;
        out << YAML::Key << "from" << YAML::Value << YAML::Flow << YAML::BeginSeq << v.gradientFrom.x << v.gradientFrom.y << YAML::EndSeq;
        out << YAML::Key << "to" << YAML::Value << YAML::Flow << YAML::BeginSeq << v.gradientTo.x << v.gradientTo.y << YAML::EndSeq;
        out << YAML::Key << "from_color" << YAML::Value;
        EmitColor(out, v.gradientFromColor);
        out << YAML::Key << "to_color" << YAML::Value;
        EmitColor(out, v.gradientToColor);
        out << YAML::EndMap;
      }
      out << YAML::EndMap;
    }

    if (entity.HasComponent<ParallaxComponent>()) {
      const auto &parallax = entity.GetComponent<ParallaxComponent>();
      out << YAML::Key << "Parallax" << YAML::BeginMap;
      out << YAML::Key << "factor" << YAML::Value << YAML::Flow
          << YAML::BeginSeq << parallax.factor.x << parallax.factor.y
          << YAML::EndSeq;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<AudioSourceComponent>()) {
      const auto &audio = entity.GetComponent<AudioSourceComponent>();
      out << YAML::Key << "AudioSource" << YAML::BeginMap;
      out << YAML::Key << "clip" << YAML::Value << audio.clip;
      out << YAML::Key << "volume" << YAML::Value << audio.volume;
      out << YAML::Key << "pitch" << YAML::Value << audio.pitch;
      out << YAML::Key << "loop" << YAML::Value << audio.loop;
      out << YAML::Key << "play_on_start" << YAML::Value << audio.playOnStart;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<ScriptComponent>()) {
      const auto &script = entity.GetComponent<ScriptComponent>();
      if (!script.LuaPath.empty()) {
        out << YAML::Key << "Script" << YAML::BeginMap;
        out << YAML::Key << "lua" << YAML::Value << script.LuaPath;
        out << YAML::EndMap;
      }
    }

    if (entity.HasComponent<TriggerComponent>()) {
      const auto &trigger = entity.GetComponent<TriggerComponent>();
      out << YAML::Key << "Trigger" << YAML::BeginMap;
      out << YAML::Key << "size" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << trigger.width << trigger.height << YAML::EndSeq;
      out << YAML::Key << "offset" << YAML::Value << YAML::Flow
          << YAML::BeginSeq << trigger.offsetX << trigger.offsetY
          << YAML::EndSeq;
      if (!trigger.filter.empty())
        out << YAML::Key << "filter" << YAML::Value << trigger.filter;
      out << YAML::Key << "enabled" << YAML::Value << trigger.enabled;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<TimerComponent>()) {
      const auto &timer = entity.GetComponent<TimerComponent>();
      out << YAML::Key << "Timer" << YAML::BeginMap;
      out << YAML::Key << "interval" << YAML::Value << timer.interval;
      out << YAML::Key << "repeat" << YAML::Value << timer.repeat;
      out << YAML::Key << "running" << YAML::Value << timer.running;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<PathFollowerComponent>()) {
      const auto &path = entity.GetComponent<PathFollowerComponent>();
      out << YAML::Key << "PathFollower" << YAML::BeginMap;
      out << YAML::Key << "speed" << YAML::Value << path.speed;
      out << YAML::Key << "wait" << YAML::Value << path.waitTime;
      out << YAML::Key << "mode" << YAML::Value
          << (path.mode == PathMode::Once   ? "once"
              : path.mode == PathMode::Loop ? "loop"
                                            : "pingpong");
      out << YAML::Key << "relative" << YAML::Value << path.relative;
      out << YAML::Key << "playing" << YAML::Value << path.playing;
      out << YAML::Key << "face_direction" << YAML::Value << path.faceDirection;
      out << YAML::Key << "points" << YAML::Value << YAML::BeginSeq;
      for (const auto &point : path.points)
        out << YAML::Flow << YAML::BeginSeq << point.x << point.y
            << YAML::EndSeq;
      out << YAML::EndSeq;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<HealthComponent>()) {
      const auto &health = entity.GetComponent<HealthComponent>();
      out << YAML::Key << "Health" << YAML::BeginMap;
      out << YAML::Key << "max" << YAML::Value << health.max;
      out << YAML::Key << "current" << YAML::Value << health.current;
      out << YAML::Key << "invulnerability" << YAML::Value
          << health.invulnerability;
      out << YAML::Key << "destroy_on_death" << YAML::Value
          << health.destroyOnDeath;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<DamageComponent>()) {
      const auto &damage = entity.GetComponent<DamageComponent>();
      out << YAML::Key << "Damage" << YAML::BeginMap;
      out << YAML::Key << "amount" << YAML::Value << damage.amount;
      out << YAML::Key << "interval" << YAML::Value << damage.interval;
      if (!damage.filter.empty())
        out << YAML::Key << "filter" << YAML::Value << damage.filter;
      out << YAML::Key << "knockback" << YAML::Value << damage.knockback;
      out << YAML::Key << "destroy_on_hit" << YAML::Value << damage.destroyOnHit;
      out << YAML::Key << "enabled" << YAML::Value << damage.enabled;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<OscillatorComponent>()) {
      const auto &osc = entity.GetComponent<OscillatorComponent>();
      out << YAML::Key << "Oscillator" << YAML::BeginMap;
      out << YAML::Key << "move" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << osc.move.x << osc.move.y << YAML::EndSeq;
      out << YAML::Key << "frequency" << YAML::Value << osc.frequency;
      out << YAML::Key << "phase" << YAML::Value << osc.phase;
      out << YAML::Key << "swing" << YAML::Value << osc.swing;
      out << YAML::Key << "spin" << YAML::Value << osc.spin;
      out << YAML::Key << "pulse" << YAML::Value << osc.pulse;
      out << YAML::Key << "playing" << YAML::Value << osc.playing;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<SpawnerComponent>()) {
      const auto &spawner = entity.GetComponent<SpawnerComponent>();
      out << YAML::Key << "Spawner" << YAML::BeginMap;
      out << YAML::Key << "prototype" << YAML::Value << spawner.prototype;
      out << YAML::Key << "interval" << YAML::Value << spawner.interval;
      out << YAML::Key << "per_spawn" << YAML::Value << spawner.perSpawn;
      out << YAML::Key << "max_alive" << YAML::Value << spawner.maxAlive;
      out << YAML::Key << "total_limit" << YAML::Value << spawner.totalLimit;
      out << YAML::Key << "offset" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << spawner.offset.x << spawner.offset.y << YAML::EndSeq;
      out << YAML::Key << "spread" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << spawner.spread.x << spawner.spread.y << YAML::EndSeq;
      out << YAML::Key << "velocity" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << spawner.velocity.x << spawner.velocity.y << YAML::EndSeq;
      out << YAML::Key << "lifetime" << YAML::Value << spawner.lifetime;
      out << YAML::Key << "hide_prototype" << YAML::Value << spawner.hidePrototype;
      out << YAML::Key << "spawn_on_start" << YAML::Value << spawner.spawnOnStart;
      out << YAML::Key << "playing" << YAML::Value << spawner.playing;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<CharacterControllerComponent>()) {
      const auto &cc = entity.GetComponent<CharacterControllerComponent>();
      out << YAML::Key << "CharacterController" << YAML::BeginMap;
      out << YAML::Key << "mode" << YAML::Value
          << (cc.mode == ControllerMode::TopDown ? "top_down" : "platformer");
      out << YAML::Key << "player_input" << YAML::Value << cc.playerInput;
      out << YAML::Key << "size" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << cc.size.x << cc.size.y << YAML::EndSeq;
      out << YAML::Key << "offset" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << cc.offset.x << cc.offset.y << YAML::EndSeq;
      out << YAML::Key << "move_speed" << YAML::Value << cc.moveSpeed;
      out << YAML::Key << "acceleration" << YAML::Value << cc.acceleration;
      out << YAML::Key << "deceleration" << YAML::Value << cc.deceleration;
      out << YAML::Key << "air_control" << YAML::Value << cc.airControl;
      out << YAML::Key << "gravity" << YAML::Value << cc.gravity;
      out << YAML::Key << "max_fall_speed" << YAML::Value << cc.maxFallSpeed;
      out << YAML::Key << "jump_speed" << YAML::Value << cc.jumpSpeed;
      out << YAML::Key << "jump_cut" << YAML::Value << cc.jumpCut;
      out << YAML::Key << "coyote_time" << YAML::Value << cc.coyoteTime;
      out << YAML::Key << "jump_buffer" << YAML::Value << cc.jumpBuffer;
      out << YAML::Key << "max_jumps" << YAML::Value << cc.maxJumps;
      out << YAML::Key << "solid_tag" << YAML::Value << cc.solidTag;
      out << YAML::Key << "one_way_tag" << YAML::Value << cc.oneWayTag;
      out << YAML::Key << "collide_tilemaps" << YAML::Value << cc.collideTilemaps;
      out << YAML::Key << "flip_sprite" << YAML::Value << cc.flipSprite;
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
        if (!tilemap.tileset->GetSourcePath().empty())
          out << YAML::Key << "tileset" << YAML::Value
              << tilemap.tileset->GetSourcePath();
        out << YAML::Key << "texture" << YAML::Value
            << tilemap.tileset->GetTexture()->GetVFSPath();
        out << YAML::Key << "tile_width" << YAML::Value
            << tilemap.tileset->GetTileWidth();
        out << YAML::Key << "tile_height" << YAML::Value
            << tilemap.tileset->GetTileHeight();
        out << YAML::Key << "margin" << YAML::Value
            << tilemap.tileset->GetMargin();
        out << YAML::Key << "spacing" << YAML::Value
            << tilemap.tileset->GetSpacing();
      }
      out << YAML::Key << "tint" << YAML::Value;
      EmitColor(out, tilemap.tint);
      out << YAML::Key << "visible" << YAML::Value << tilemap.visible;
      out << YAML::Key << "solid" << YAML::Value << tilemap.solid;

      out << YAML::Key << "chunks" << YAML::Value << YAML::BeginSeq;
      std::vector<std::int64_t> chunkKeys;
      chunkKeys.reserve(tilemap.chunks.size());
      for (const auto &entry : tilemap.chunks)
        chunkKeys.push_back(entry.first);
      std::sort(chunkKeys.begin(), chunkKeys.end());
      for (const std::int64_t key : chunkKeys) {
        const TileChunk &chunk = tilemap.chunks.at(key);
        const auto packed = static_cast<std::uint64_t>(key);
        const auto chunkX = static_cast<std::int32_t>(packed >> 32);
        const auto chunkY = static_cast<std::int32_t>(static_cast<std::uint32_t>(packed));

        out << YAML::BeginMap;
        out << YAML::Key << "x" << YAML::Value << chunkX;
        out << YAML::Key << "y" << YAML::Value << chunkY;
        out << YAML::Key << "tiles" << YAML::BeginSeq;
        for (int localY = 0; localY < CHUNK_SIZE; ++localY) {
          for (int localX = 0; localX < CHUNK_SIZE; ++localX) {
            const Tile &tile = chunk.tiles[static_cast<size_t>(localY * CHUNK_SIZE + localX)];
            if (tile.id < 0)
              continue;
            out << YAML::Flow << YAML::BeginSeq << localX << localY << tile.id;
            if (tile.flags)
              out << static_cast<int>(tile.flags);
            out << YAML::EndSeq;
          }
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;
      }
      out << YAML::EndSeq;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<ParticleEmitterComponent>()) {
      const auto &pe = entity.GetComponent<ParticleEmitterComponent>();
      out << YAML::Key << "ParticleEmitter" << YAML::BeginMap;
      out << YAML::Key << "playing" << YAML::Value << pe.playing;
      out << YAML::Key << "effect" << YAML::Value;
      ParticleSerializer::Emit(out, pe.effect);
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
  if (!FileSystem::WriteText(realPath, yaml)) {
    CORE_ERROR("Could not write '{}'", realPath);
    return false;
  }
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
  if (!data.IsMap() || !data["Scene"]) {
    CORE_ERROR("Not a scene file (no 'Scene' key)");
    return false;
  }
  if (auto entities = data["Entities"]; entities && !entities.IsSequence() && !entities.IsNull()) {
    CORE_ERROR("Scene 'Entities' must be a list");
    return false;
  }
  try {
    return Load(data, true, created);
  } catch (const std::exception &e) {
    CORE_ERROR("Failed to load scene: {}", e.what());
    return false;
  }
}

std::vector<entt::entity>
SceneSerializer::DeserializeEntities(const std::string &yaml) {
  std::vector<entt::entity> created;
  try {
    Load(YAML::Load(yaml), false, &created);
  } catch (const std::exception &e) {
    CORE_ERROR("Failed to parse entities: {}", e.what());
  }
  return created;
}

bool SceneSerializer::Load(const YAML::Node &data, bool applySettings,
                           std::vector<entt::entity> *created) {
  if (!data["Scene"])
    return false;

  int version = 0;
  if (data["Version"])
    TryGet(data, "Version", version);
  if (version > FormatVersion)
    CORE_WARN("Scene format {} is newer than this engine ({}); unknown data is skipped", version, FormatVersion);

  if (applySettings) {
    std::string sceneName;
    if (TryGet(data, "Scene", sceneName))
      m_Scene->SetName(sceneName);

    if (auto settings = data["Settings"]) {
      auto bg = settings["background"];
      if (bg && bg.IsSequence() && bg.size() >= 4)
        m_Scene->SetBackgroundColor({bg[0].as<float>(), bg[1].as<float>(),
                                     bg[2].as<float>(), bg[3].as<float>()});

      SceneLighting lighting;
      if (auto l = settings["lighting"]) {
        if (l["enabled"])
          TryGet(l, "enabled", lighting.Enabled);
        auto a = l["ambient"];
        if (a && a.IsSequence() && a.size() >= 4)
          lighting.Ambient = {a[0].as<float>(), a[1].as<float>(),
                              a[2].as<float>(), a[3].as<float>()};
      }
      m_Scene->GetLighting() = lighting;

      BloomSettings bloom;
      if (auto b = settings["bloom"]) {
        if (b["enabled"])
          TryGet(b, "enabled", bloom.Enabled);
        if (b["threshold"])
          TryGet(b, "threshold", bloom.Threshold);
        if (b["knee"])
          TryGet(b, "knee", bloom.Knee);
        if (b["intensity"])
          TryGet(b, "intensity", bloom.Intensity);
        if (b["radius"])
          TryGet(b, "radius", bloom.Radius);
      }
      m_Scene->GetBloom() = bloom;
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
      } catch (const std::exception &e) {
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
      if (auto r = s["region"]; r && r.IsSequence() && r.size() >= 4) {
        sc.useRegion = true;
        sc.region = {r[0].as<float>(), r[1].as<float>(), r[2].as<float>(),
                     r[3].as<float>()};
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
      if (cam["follow"])
        TryGet(cam, "follow", cc.followTarget);
      ReadVec2(cam["follow_offset"], cc.followOffset.x, cc.followOffset.y);
      if (cam["smoothing"])
        TryGet(cam, "smoothing", cc.followSmoothing);
      ReadVec2(cam["dead_zone"], cc.deadZone.x, cc.deadZone.y);
      if (auto b = cam["bounds"]; b && b.IsSequence() && b.size() >= 4) {
        cc.useBounds = true;
        cc.boundsMin = {b[0].as<float>(), b[1].as<float>()};
        cc.boundsMax = {b[2].as<float>(), b[3].as<float>()};
      }
      if (cam["shake_strength"])
        TryGet(cam, "shake_strength", cc.shakeStrength);
    });

    component("Light", [&](const YAML::Node &l) {
      auto &light = entity.AddComponent<LightComponent>();
      std::string type;
      if (l["type"])
        TryGet(l, "type", type);
      light.type = type == "spot" ? LightType::Spot : LightType::Point;
      ReadColor(l["color"], light.color);
      if (l["intensity"])
        TryGet(l, "intensity", light.intensity);
      if (l["radius"])
        TryGet(l, "radius", light.radius);
      if (l["falloff"])
        TryGet(l, "falloff", light.falloff);
      if (l["inner_angle"])
        TryGet(l, "inner_angle", light.innerAngle);
      if (l["outer_angle"])
        TryGet(l, "outer_angle", light.outerAngle);
      if (l["flicker"])
        TryGet(l, "flicker", light.flicker);
      if (l["flicker_speed"])
        TryGet(l, "flicker_speed", light.flickerSpeed);
      if (l["enabled"])
        TryGet(l, "enabled", light.enabled);
    });

    component("Glow", [&](const YAML::Node &g) {
      auto &glow = entity.AddComponent<GlowComponent>();
      ReadColor(g["color"], glow.color);
      if (g["intensity"])
        TryGet(g, "intensity", glow.intensity);
      if (g["spread"])
        TryGet(g, "spread", glow.spread);
      if (g["enabled"])
        TryGet(g, "enabled", glow.enabled);
    });

    component("VectorShape", [&](const YAML::Node &n) {
      auto &v = entity.AddComponent<VectorShapeComponent>();
      ReadPath(n["path"], v.path);
      ReadPath(n["morph_target"], v.morphTarget);
      if (n["morph"])
        TryGet(n, "morph", v.morph);
      if (n["fill"])
        TryGet(n, "fill", v.fill);
      ReadColor(n["fill_color"], v.fillColor);
      if (n["stroke"])
        TryGet(n, "stroke", v.stroke);
      ReadColor(n["stroke_color"], v.strokeColor);
      if (n["stroke_width"])
        TryGet(n, "stroke_width", v.strokeWidth);
      if (n["cap"]) {
        const std::string cap = n["cap"].as<std::string>();
        v.cap = cap == "butt" ? StrokeCap::Butt : cap == "square" ? StrokeCap::Square : StrokeCap::Round;
      }
      if (n["join"]) {
        const std::string join = n["join"].as<std::string>();
        v.join = join == "miter" ? StrokeJoin::Miter : join == "bevel" ? StrokeJoin::Bevel : StrokeJoin::Round;
      }
      if (n["miter_limit"])
        TryGet(n, "miter_limit", v.miterLimit);
      if (n["trim"] && n["trim"].IsSequence() && n["trim"].size() >= 2) {
        v.trimStart = n["trim"][0].as<float>();
        v.trimEnd = n["trim"][1].as<float>();
        if (n["trim"].size() >= 3)
          v.trimOffset = n["trim"][2].as<float>();
      }
      if (n["dash"] && n["dash"].IsSequence() && n["dash"].size() >= 2) {
        v.dashLength = n["dash"][0].as<float>();
        v.dashGap = n["dash"][1].as<float>();
        if (n["dash"].size() >= 3)
          v.dashOffset = n["dash"][2].as<float>();
      }
      if (const YAML::Node g = n["gradient"]) {
        v.gradient = true;
        ReadVec2(g["from"], v.gradientFrom.x, v.gradientFrom.y);
        ReadVec2(g["to"], v.gradientTo.x, v.gradientTo.y);
        ReadColor(g["from_color"], v.gradientFromColor);
        ReadColor(g["to_color"], v.gradientToColor);
      }
    });

    component("Parallax", [&](const YAML::Node &p) {
      auto &parallax = entity.AddComponent<ParallaxComponent>();
      ReadVec2(p["factor"], parallax.factor.x, parallax.factor.y);
    });

    component("AudioSource", [&](const YAML::Node &a) {
      auto &audio = entity.AddComponent<AudioSourceComponent>();
      if (a["clip"])
        TryGet(a, "clip", audio.clip);
      if (a["volume"])
        TryGet(a, "volume", audio.volume);
      if (a["pitch"])
        TryGet(a, "pitch", audio.pitch);
      if (a["loop"])
        TryGet(a, "loop", audio.loop);
      if (a["play_on_start"])
        TryGet(a, "play_on_start", audio.playOnStart);
    });

    component("Script", [&](const YAML::Node &sn) {
      std::string lua;
      if (sn["lua"])
        TryGet(sn, "lua", lua);
      if (lua.empty())
        return;
      entity.AddComponent<ScriptComponent>().BindLua(lua);
    });

    component("Trigger", [&](const YAML::Node &t) {
      auto &trigger = entity.AddComponent<TriggerComponent>();
      ReadVec2(t["size"], trigger.width, trigger.height);
      ReadVec2(t["offset"], trigger.offsetX, trigger.offsetY);
      if (t["filter"])
        TryGet(t, "filter", trigger.filter);
      if (t["enabled"])
        TryGet(t, "enabled", trigger.enabled);
    });

    component("Timer", [&](const YAML::Node &t) {
      auto &timer = entity.AddComponent<TimerComponent>();
      if (t["interval"])
        TryGet(t, "interval", timer.interval);
      if (t["repeat"])
        TryGet(t, "repeat", timer.repeat);
      if (t["running"])
        TryGet(t, "running", timer.running);
    });

    component("PathFollower", [&](const YAML::Node &p) {
      auto &path = entity.AddComponent<PathFollowerComponent>();
      if (p["speed"])
        TryGet(p, "speed", path.speed);
      if (p["wait"])
        TryGet(p, "wait", path.waitTime);
      if (p["mode"]) {
        std::string mode;
        TryGet(p, "mode", mode);
        path.mode = mode == "once"   ? PathMode::Once
                    : mode == "loop" ? PathMode::Loop
                                     : PathMode::PingPong;
      }
      if (p["relative"])
        TryGet(p, "relative", path.relative);
      if (p["playing"])
        TryGet(p, "playing", path.playing);
      if (p["face_direction"])
        TryGet(p, "face_direction", path.faceDirection);
      if (auto points = p["points"]; points && points.IsSequence()) {
        for (const auto &pointNode : points) {
          float x = 0.0f;
          float y = 0.0f;
          if (ReadVec2(pointNode, x, y))
            path.points.emplace_back(x, y);
        }
      }
    });

    component("CharacterController", [&](const YAML::Node &n) {
      auto &cc = entity.AddComponent<CharacterControllerComponent>();
      std::string mode;
      if (n["mode"] && TryGet(n, "mode", mode))
        cc.mode = mode == "top_down" ? ControllerMode::TopDown : ControllerMode::Platformer;
      if (n["player_input"]) TryGet(n, "player_input", cc.playerInput);
      ReadVec2(n["size"], cc.size.x, cc.size.y);
      ReadVec2(n["offset"], cc.offset.x, cc.offset.y);
      if (n["move_speed"]) TryGet(n, "move_speed", cc.moveSpeed);
      if (n["acceleration"]) TryGet(n, "acceleration", cc.acceleration);
      if (n["deceleration"]) TryGet(n, "deceleration", cc.deceleration);
      if (n["air_control"]) TryGet(n, "air_control", cc.airControl);
      if (n["gravity"]) TryGet(n, "gravity", cc.gravity);
      if (n["max_fall_speed"]) TryGet(n, "max_fall_speed", cc.maxFallSpeed);
      if (n["jump_speed"]) TryGet(n, "jump_speed", cc.jumpSpeed);
      if (n["jump_cut"]) TryGet(n, "jump_cut", cc.jumpCut);
      if (n["coyote_time"]) TryGet(n, "coyote_time", cc.coyoteTime);
      if (n["jump_buffer"]) TryGet(n, "jump_buffer", cc.jumpBuffer);
      if (n["max_jumps"]) TryGet(n, "max_jumps", cc.maxJumps);
      if (n["solid_tag"]) TryGet(n, "solid_tag", cc.solidTag);
      if (n["one_way_tag"]) TryGet(n, "one_way_tag", cc.oneWayTag);
      if (n["collide_tilemaps"]) TryGet(n, "collide_tilemaps", cc.collideTilemaps);
      if (n["flip_sprite"]) TryGet(n, "flip_sprite", cc.flipSprite);
    });

    component("Health", [&](const YAML::Node &h) {
      auto &health = entity.AddComponent<HealthComponent>();
      if (h["max"])
        TryGet(h, "max", health.max);
      health.current = health.max;
      if (h["current"])
        TryGet(h, "current", health.current);
      if (h["invulnerability"])
        TryGet(h, "invulnerability", health.invulnerability);
      if (h["destroy_on_death"])
        TryGet(h, "destroy_on_death", health.destroyOnDeath);
      health.dead = health.current <= 0.0f;
    });

    component("Damage", [&](const YAML::Node &d) {
      auto &damage = entity.AddComponent<DamageComponent>();
      if (d["amount"]) TryGet(d, "amount", damage.amount);
      if (d["interval"]) TryGet(d, "interval", damage.interval);
      if (d["filter"]) TryGet(d, "filter", damage.filter);
      if (d["knockback"]) TryGet(d, "knockback", damage.knockback);
      if (d["destroy_on_hit"]) TryGet(d, "destroy_on_hit", damage.destroyOnHit);
      if (d["enabled"]) TryGet(d, "enabled", damage.enabled);
    });

    component("Oscillator", [&](const YAML::Node &o) {
      auto &osc = entity.AddComponent<OscillatorComponent>();
      ReadVec2(o["move"], osc.move.x, osc.move.y);
      if (o["frequency"]) TryGet(o, "frequency", osc.frequency);
      if (o["phase"]) TryGet(o, "phase", osc.phase);
      if (o["swing"]) TryGet(o, "swing", osc.swing);
      if (o["spin"]) TryGet(o, "spin", osc.spin);
      if (o["pulse"]) TryGet(o, "pulse", osc.pulse);
      if (o["playing"]) TryGet(o, "playing", osc.playing);
    });

    component("Spawner", [&](const YAML::Node &n) {
      auto &spawner = entity.AddComponent<SpawnerComponent>();
      if (n["prototype"]) TryGet(n, "prototype", spawner.prototype);
      if (n["interval"]) TryGet(n, "interval", spawner.interval);
      if (n["per_spawn"]) TryGet(n, "per_spawn", spawner.perSpawn);
      if (n["max_alive"]) TryGet(n, "max_alive", spawner.maxAlive);
      if (n["total_limit"]) TryGet(n, "total_limit", spawner.totalLimit);
      ReadVec2(n["offset"], spawner.offset.x, spawner.offset.y);
      ReadVec2(n["spread"], spawner.spread.x, spawner.spread.y);
      ReadVec2(n["velocity"], spawner.velocity.x, spawner.velocity.y);
      if (n["lifetime"]) TryGet(n, "lifetime", spawner.lifetime);
      if (n["hide_prototype"]) TryGet(n, "hide_prototype", spawner.hidePrototype);
      if (n["spawn_on_start"]) TryGet(n, "spawn_on_start", spawner.spawnOnStart);
      if (n["playing"]) TryGet(n, "playing", spawner.playing);
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
      if (a["playing"])
        TryGet(a, "playing", ac.playing);
      if (a["speed"])
        TryGet(a, "speed", ac.speed);
    });

    component("ParticleEmitter", [&](const YAML::Node &p) {
      auto &pe = entity.AddComponent<ParticleEmitterComponent>();
      if (p["playing"])
        TryGet(p, "playing", pe.playing);
      ParticleSerializer::Load(p["effect"], pe.effect);
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
      int margin = 0;
      int spacing = 0;
      if (tilemapNode["margin"])
        TryGet(tilemapNode, "margin", margin);
      if (tilemapNode["spacing"])
        TryGet(tilemapNode, "spacing", spacing);
      if (hasTileset && tileWidth > 0 && tileHeight > 0) {
        auto texture = TextureManager::Get().LoadTexture(texturePath, texturePath);
        if (texture) {
          tilemap.tileset = std::make_shared<Tileset>(texture, tileWidth, tileHeight, margin, spacing);
          if (tilemapNode["tileset"])
            tilemap.tileset->SetSourcePath(tilemapNode["tileset"].as<std::string>());
        }
      }
      ReadColor(tilemapNode["tint"], tilemap.tint);
      if (tilemapNode["visible"])
        TryGet(tilemapNode, "visible", tilemap.visible);
      if (tilemapNode["solid"])
        TryGet(tilemapNode, "solid", tilemap.solid);

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
            const int flags = tileNode.size() >= 4 ? tileNode[3].as<int>() : 0;
            if (localX < 0 || localX >= CHUNK_SIZE || localY < 0 ||
                localY >= CHUNK_SIZE || tileId < 0)
              continue;
            SetTile(tilemap, chunkX * CHUNK_SIZE + localX,
                    chunkY * CHUNK_SIZE + localY, tileId,
                    static_cast<uint8_t>(flags));
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
