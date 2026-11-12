#include <Engine/Core/FileSystem.h>
#include <Engine/Physics/BodyFactory.h>
#include "yaml-cpp/emittermanip.h"
#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Rendering/TextureManager.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/SceneSerializer.h>
#include <Engine/Scene/TilemapUtils.h>
#include <Engine/Particles/ParticleSerializer.h>

#include <cmath>
#include <cstdint>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>
#include <exception>
#include <memory>

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
    const AccelEngine::World &physicsWorld = m_Scene->GetPhysicsWorld();
    out << YAML::Key << "physics" << YAML::BeginMap;
    out << YAML::Key << "gravity" << YAML::Value << YAML::Flow << YAML::BeginSeq
        << physicsWorld.gravity.x << physicsWorld.gravity.y << YAML::EndSeq;
    out << YAML::Key << "substeps" << YAML::Value << physicsWorld.settings.substeps;
    out << YAML::Key << "velocity_iterations" << YAML::Value << physicsWorld.settings.velocityIterations;
    out << YAML::Key << "position_iterations" << YAML::Value << physicsWorld.settings.positionIterations;
    out << YAML::Key << "enable_sleep" << YAML::Value << physicsWorld.settings.enableSleep;
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
      out << YAML::Key << "bus" << YAML::Value
          << (audio.bus == 1 ? "music" : audio.bus == 2 ? "ui" : "sfx");
      out << YAML::EndMap;
    }

    if (entity.HasComponent<FollowComponent>()) {
      const auto &follow = entity.GetComponent<FollowComponent>();
      out << YAML::Key << "Follow" << YAML::BeginMap;
      out << YAML::Key << "target" << YAML::Value << follow.target;
      out << YAML::Key << "offset" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << follow.offset.x << follow.offset.y << YAML::EndSeq;
      out << YAML::Key << "smoothing" << YAML::Value << follow.smoothing;
      out << YAML::Key << "stop_distance" << YAML::Value << follow.stopDistance;
      out << YAML::Key << "follow_x" << YAML::Value << follow.followX;
      out << YAML::Key << "follow_y" << YAML::Value << follow.followY;
      out << YAML::Key << "enabled" << YAML::Value << follow.enabled;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<ButtonComponent>()) {
      const auto &button = entity.GetComponent<ButtonComponent>();
      out << YAML::Key << "Button" << YAML::BeginMap;
      out << YAML::Key << "enabled" << YAML::Value << button.enabled;
      out << YAML::Key << "button" << YAML::Value
          << (button.button == 1 ? "right" : button.button == 2 ? "middle" : "left");
      out << YAML::Key << "hover_scale" << YAML::Value << button.hoverScale;
      out << YAML::Key << "press_scale" << YAML::Value << button.pressScale;
      out << YAML::Key << "hover_sound" << YAML::Value << button.hoverSound;
      out << YAML::Key << "click_sound" << YAML::Value << button.clickSound;
      out << YAML::Key << "volume" << YAML::Value << button.volume;
      out << YAML::Key << "padding" << YAML::Value << button.padding;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<ShaderCanvasComponent>()) {
      const auto &canvas = entity.GetComponent<ShaderCanvasComponent>();
      static const char *types[] = {"float", "vec2", "vec3", "vec4", "color", "texture", "int", "bool"};
      out << YAML::Key << "ShaderCanvas" << YAML::BeginMap;
      out << YAML::Key << "shader" << YAML::Value << canvas.shader;
      out << YAML::Key << "scope" << YAML::Value
          << (canvas.scope == 1 ? "children" : canvas.scope == 2 ? "z_range" : "scene");
      out << YAML::Key << "z_range" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << canvas.zMin << canvas.zMax << YAML::EndSeq;
      out << YAML::Key << "order" << YAML::Value << canvas.order;
      out << YAML::Key << "resolution_scale" << YAML::Value << canvas.resolutionScale;
      out << YAML::Key << "progress" << YAML::Value << canvas.progress;
      out << YAML::Key << "enabled" << YAML::Value << canvas.enabled;
      out << YAML::Key << "limit_area" << YAML::Value << canvas.limitArea;
      out << YAML::Key << "area_size" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << canvas.areaSize.x << canvas.areaSize.y << YAML::EndSeq;
      if (!canvas.uniforms.empty()) {
        out << YAML::Key << "uniforms" << YAML::Value << YAML::BeginSeq;
        for (const auto &uniform : canvas.uniforms) {
          out << YAML::BeginMap;
          out << YAML::Key << "name" << YAML::Value << uniform.name;
          out << YAML::Key << "type" << YAML::Value
              << types[std::clamp(uniform.type, 0, 7)];
          if (uniform.type == 5) {
            out << YAML::Key << "texture" << YAML::Value << uniform.texturePath;
          } else {
            out << YAML::Key << "value" << YAML::Value << YAML::Flow << YAML::BeginSeq
                << uniform.value[0] << uniform.value[1] << uniform.value[2]
                << uniform.value[3] << YAML::EndSeq;
          }
          out << YAML::EndMap;
        }
        out << YAML::EndSeq;
      }
      out << YAML::EndMap;
    }

    if (entity.HasComponent<PrefabInstanceComponent>()) {
      out << YAML::Key << "PrefabInstance" << YAML::BeginMap;
      out << YAML::Key << "path" << YAML::Value
          << entity.GetComponent<PrefabInstanceComponent>().path;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<CanvasComponent>()) {
      const auto &canvas = entity.GetComponent<CanvasComponent>();
      out << YAML::Key << "Canvas" << YAML::BeginMap;
      out << YAML::Key << "reference_size" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << canvas.referenceSize.x << canvas.referenceSize.y << YAML::EndSeq;
      out << YAML::Key << "scale_mode" << YAML::Value << (canvas.scaleMode == 0 ? "constant" : "scale_with_screen");
      out << YAML::Key << "match" << YAML::Value << canvas.match;
      out << YAML::Key << "safe_area" << YAML::Value << canvas.safeArea;
      out << YAML::Key << "sort_order" << YAML::Value << canvas.sortOrder;
      out << YAML::Key << "visible" << YAML::Value << canvas.visible;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<UIRectComponent>()) {
      const auto &rect = entity.GetComponent<UIRectComponent>();
      out << YAML::Key << "UIRect" << YAML::BeginMap;
      out << YAML::Key << "anchor_min" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << rect.anchorMin.x << rect.anchorMin.y << YAML::EndSeq;
      out << YAML::Key << "anchor_max" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << rect.anchorMax.x << rect.anchorMax.y << YAML::EndSeq;
      out << YAML::Key << "pivot" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << rect.pivot.x << rect.pivot.y << YAML::EndSeq;
      out << YAML::Key << "position" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << rect.position.x << rect.position.y << YAML::EndSeq;
      out << YAML::Key << "size" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << rect.size.x << rect.size.y << YAML::EndSeq;
      out << YAML::Key << "margin" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << rect.margin.x << rect.margin.y << rect.margin.z << rect.margin.w << YAML::EndSeq;
      out << YAML::Key << "visible" << YAML::Value << rect.visible;
      out << YAML::Key << "blocks_input" << YAML::Value << rect.blocksInput;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<UIPanelComponent>()) {
      const auto &panel = entity.GetComponent<UIPanelComponent>();
      out << YAML::Key << "UIPanel" << YAML::BeginMap;
      out << YAML::Key << "color" << YAML::Value;
      EmitColor(out, panel.color);
      out << YAML::Key << "corner_radius" << YAML::Value << panel.cornerRadius;
      out << YAML::Key << "border_color" << YAML::Value;
      EmitColor(out, panel.borderColor);
      out << YAML::Key << "border_width" << YAML::Value << panel.borderWidth;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<UILabelComponent>()) {
      const auto &label = entity.GetComponent<UILabelComponent>();
      out << YAML::Key << "UILabel" << YAML::BeginMap;
      out << YAML::Key << "text" << YAML::Value << label.text;
      out << YAML::Key << "size" << YAML::Value << label.size;
      out << YAML::Key << "color" << YAML::Value;
      EmitColor(out, label.color);
      out << YAML::Key << "align" << YAML::Value << label.align;
      out << YAML::Key << "vertical_align" << YAML::Value << label.verticalAlign;
      if (!label.fontPath.empty())
        out << YAML::Key << "font" << YAML::Value << label.fontPath;
      out << YAML::Key << "shadow" << YAML::Value << label.shadow;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<UIImageComponent>()) {
      const auto &image = entity.GetComponent<UIImageComponent>();
      out << YAML::Key << "UIImage" << YAML::BeginMap;
      if (image.texture) {
        out << YAML::Key << "texture" << YAML::Value << image.texture->GetVFSPath();
        out << YAML::Key << "filter" << YAML::Value
            << (image.texture->GetFilter() == TextureFilter::Nearest ? "nearest" : "linear");
      }
      out << YAML::Key << "tint" << YAML::Value;
      EmitColor(out, image.tint);
      out << YAML::Key << "preserve_aspect" << YAML::Value << image.preserveAspect;
      if (image.useRegion)
        out << YAML::Key << "region" << YAML::Value << YAML::Flow << YAML::BeginSeq
            << image.region.x << image.region.y << image.region.w << image.region.h << YAML::EndSeq;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<UIButtonComponent>()) {
      const auto &button = entity.GetComponent<UIButtonComponent>();
      out << YAML::Key << "UIButton" << YAML::BeginMap;
      out << YAML::Key << "enabled" << YAML::Value << button.enabled;
      out << YAML::Key << "focusable" << YAML::Value << button.focusable;
      out << YAML::Key << "hover_tint" << YAML::Value;
      EmitColor(out, button.hoverTint);
      out << YAML::Key << "press_tint" << YAML::Value;
      EmitColor(out, button.pressTint);
      out << YAML::Key << "disabled_tint" << YAML::Value;
      EmitColor(out, button.disabledTint);
      out << YAML::Key << "hover_scale" << YAML::Value << button.hoverScale;
      out << YAML::Key << "press_scale" << YAML::Value << button.pressScale;
      out << YAML::Key << "hover_sound" << YAML::Value << button.hoverSound;
      out << YAML::Key << "click_sound" << YAML::Value << button.clickSound;
      out << YAML::Key << "volume" << YAML::Value << button.volume;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<UILayoutComponent>()) {
      const auto &layout = entity.GetComponent<UILayoutComponent>();
      out << YAML::Key << "UILayout" << YAML::BeginMap;
      out << YAML::Key << "direction" << YAML::Value << (layout.direction == 0 ? "row" : "column");
      out << YAML::Key << "spacing" << YAML::Value << layout.spacing;
      out << YAML::Key << "padding" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << layout.padding.x << layout.padding.y << layout.padding.z << layout.padding.w << YAML::EndSeq;
      out << YAML::Key << "justify" << YAML::Value << layout.justify;
      out << YAML::Key << "align" << YAML::Value << layout.align;
      out << YAML::Key << "stretch" << YAML::Value << layout.stretch;
      out << YAML::Key << "enabled" << YAML::Value << layout.enabled;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<TeleporterComponent>()) {
      const auto &portal = entity.GetComponent<TeleporterComponent>();
      out << YAML::Key << "Teleporter" << YAML::BeginMap;
      out << YAML::Key << "target" << YAML::Value << portal.target;
      out << YAML::Key << "offset" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << portal.offset.x << portal.offset.y << YAML::EndSeq;
      out << YAML::Key << "sound" << YAML::Value << portal.sound;
      out << YAML::Key << "keep_velocity" << YAML::Value << portal.keepVelocity;
      out << YAML::Key << "enabled" << YAML::Value << portal.enabled;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<CameraZoneComponent>()) {
      const auto &zone = entity.GetComponent<CameraZoneComponent>();
      out << YAML::Key << "CameraZone" << YAML::BeginMap;
      out << YAML::Key << "zoom" << YAML::Value << zone.zoom;
      out << YAML::Key << "speed" << YAML::Value << zone.speed;
      out << YAML::Key << "set_bounds" << YAML::Value << zone.setBounds;
      out << YAML::Key << "bounds_min" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << zone.boundsMin.x << zone.boundsMin.y << YAML::EndSeq;
      out << YAML::Key << "bounds_max" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << zone.boundsMax.x << zone.boundsMax.y << YAML::EndSeq;
      out << YAML::Key << "restore_on_exit" << YAML::Value << zone.restoreOnExit;
      out << YAML::Key << "enabled" << YAML::Value << zone.enabled;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<WorldBoundsComponent>()) {
      const auto &bounds = entity.GetComponent<WorldBoundsComponent>();
      out << YAML::Key << "WorldBounds" << YAML::BeginMap;
      out << YAML::Key << "min" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << bounds.min.x << bounds.min.y << YAML::EndSeq;
      out << YAML::Key << "max" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << bounds.max.x << bounds.max.y << YAML::EndSeq;
      out << YAML::Key << "mode" << YAML::Value
          << (bounds.mode == BoundsMode::Wrap     ? "wrap"
              : bounds.mode == BoundsMode::Bounce ? "bounce"
              : bounds.mode == BoundsMode::Destroy ? "destroy"
                                                   : "clamp");
      out << YAML::Key << "margin" << YAML::Value << bounds.margin;
      out << YAML::Key << "enabled" << YAML::Value << bounds.enabled;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<PickableComponent>()) {
      const auto &pickable = entity.GetComponent<PickableComponent>();
      out << YAML::Key << "Pickable" << YAML::BeginMap;
      out << YAML::Key << "enabled" << YAML::Value << pickable.enabled;
      out << YAML::Key << "button" << YAML::Value
          << (pickable.button == 1 ? "right" : pickable.button == 2 ? "middle" : "left");
      out << YAML::Key << "smoothing" << YAML::Value << pickable.smoothing;
      out << YAML::Key << "throw_strength" << YAML::Value << pickable.throwStrength;
      out << YAML::Key << "lift_scale" << YAML::Value << pickable.liftScale;
      out << YAML::Key << "bring_to_front" << YAML::Value << pickable.bringToFront;
      out << YAML::Key << "keep_offset" << YAML::Value << pickable.keepOffset;
      out << YAML::Key << "padding" << YAML::Value << pickable.padding;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<PickupComponent>()) {
      const auto &pickup = entity.GetComponent<PickupComponent>();
      out << YAML::Key << "Pickup" << YAML::BeginMap;
      out << YAML::Key << "key" << YAML::Value << pickup.key;
      out << YAML::Key << "amount" << YAML::Value << pickup.amount;
      out << YAML::Key << "sound" << YAML::Value << pickup.sound;
      out << YAML::Key << "volume" << YAML::Value << pickup.volume;
      out << YAML::Key << "destroy_on_pickup" << YAML::Value << pickup.destroyOnPickup;
      out << YAML::Key << "enabled" << YAML::Value << pickup.enabled;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<CheckpointComponent>()) {
      const auto &checkpoint = entity.GetComponent<CheckpointComponent>();
      out << YAML::Key << "Checkpoint" << YAML::BeginMap;
      out << YAML::Key << "offset" << YAML::Value << YAML::Flow << YAML::BeginSeq
          << checkpoint.offset.x << checkpoint.offset.y << YAML::EndSeq;
      out << YAML::Key << "sound" << YAML::Value << checkpoint.sound;
      out << YAML::Key << "enabled" << YAML::Value << checkpoint.enabled;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<RespawnComponent>()) {
      const auto &respawn = entity.GetComponent<RespawnComponent>();
      out << YAML::Key << "Respawn" << YAML::BeginMap;
      out << YAML::Key << "delay" << YAML::Value << respawn.delay;
      out << YAML::Key << "use_kill_y" << YAML::Value << respawn.useKillY;
      out << YAML::Key << "kill_y" << YAML::Value << respawn.killY;
      out << YAML::Key << "restore_health" << YAML::Value << respawn.restoreHealth;
      out << YAML::EndMap;
    }

    if (entity.HasComponent<MusicComponent>()) {
      const auto &music = entity.GetComponent<MusicComponent>();
      out << YAML::Key << "Music" << YAML::BeginMap;
      out << YAML::Key << "tracks" << YAML::Value << YAML::BeginSeq;
      for (const auto &track : music.tracks)
        out << track;
      out << YAML::EndSeq;
      out << YAML::Key << "volume" << YAML::Value << music.volume;
      out << YAML::Key << "fade_in" << YAML::Value << music.fadeIn;
      out << YAML::Key << "fade_out" << YAML::Value << music.fadeOut;
      out << YAML::Key << "shuffle" << YAML::Value << music.shuffle;
      out << YAML::Key << "loop_playlist" << YAML::Value << music.loopPlaylist;
      out << YAML::Key << "play_on_start" << YAML::Value << music.playOnStart;
      out << YAML::Key << "on_trigger" << YAML::Value << music.onTrigger;
      out << YAML::Key << "stop_on_exit" << YAML::Value << music.stopOnExit;
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
        using AccelEngine::ShapeType;
        out << YAML::Key << "Physics" << YAML::BeginMap;

        const char *shape = body->shapeType == ShapeType::CIRCLE    ? "CIRCLE"
                            : body->shapeType == ShapeType::AABB    ? "AABB"
                            : body->shapeType == ShapeType::POLYGON ? "POLYGON"
                            : body->shapeType == ShapeType::CAPSULE ? "CAPSULE"
                                                                    : "COMPOUND";
        out << YAML::Key << "shape" << YAML::Value << shape;
        out << YAML::Key << "angular_velocity" << YAML::Value << body->rotation;
        out << YAML::Key << "velocity" << YAML::Value << YAML::Flow
            << YAML::BeginSeq << body->velocity.x << body->velocity.y
            << YAML::EndSeq;
        out << YAML::Key << "mass" << YAML::Value
            << (body->inverseMass > 0.0f ? 1.0f / body->inverseMass : 0.0f);

        if (body->shapeType == ShapeType::CIRCLE) {
          out << YAML::Key << "radius" << YAML::Value << body->circle.radius;
        } else if (body->shapeType == ShapeType::AABB) {
          out << YAML::Key << "half_size" << YAML::Value << YAML::Flow
              << YAML::BeginSeq << body->aabb.halfSize.x << body->aabb.halfSize.y
              << YAML::EndSeq;
        } else {
          out << YAML::Key << "parts" << YAML::Value << YAML::BeginSeq;
          for (const auto &part : body->parts) {
            out << YAML::BeginMap;
            out << YAML::Key << "radius" << YAML::Value << part.radius;
            out << YAML::Key << "vertices" << YAML::Value << YAML::BeginSeq;
            for (int i = 0; i < part.count; ++i)
              out << YAML::Flow << YAML::BeginSeq << part.v[i].x << part.v[i].y
                  << YAML::EndSeq;
            out << YAML::EndSeq << YAML::EndMap;
          }
          out << YAML::EndSeq;
        }

        out << YAML::Key << "restitution" << YAML::Value << body->restitution;
        out << YAML::Key << "static_friction" << YAML::Value << body->staticFriction;
        out << YAML::Key << "dynamic_friction" << YAML::Value << body->dynamicFriction;
        out << YAML::Key << "linear_damping" << YAML::Value << body->linearDamping;
        out << YAML::Key << "angular_damping" << YAML::Value << body->angularDamping;
        out << YAML::Key << "lock_rotation" << YAML::Value << body->lockRotation;
        out << YAML::Key << "lock_position" << YAML::Value << body->lockPosition;
        out << YAML::Key << "ignore_gravity" << YAML::Value << body->ignoreGravity;
        out << YAML::Key << "can_sleep" << YAML::Value << body->canSleep;
        out << YAML::Key << "category" << YAML::Value << static_cast<int>(body->categoryBits);
        out << YAML::Key << "mask" << YAML::Value << static_cast<int>(body->maskBits);
        out << YAML::Key << "group" << YAML::Value << static_cast<int>(body->groupIndex);
        out << YAML::Key << "sensor" << YAML::Value << body->isSensor;
        out << YAML::Key << "one_way" << YAML::Value << body->oneWay;
        out << YAML::Key << "one_way_normal" << YAML::Value << YAML::Flow
            << YAML::BeginSeq << body->oneWayNormal.x << body->oneWayNormal.y
            << YAML::EndSeq;

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
  if (!DeserializeFromString(buffer.str()))
    return false;
  m_Scene->SetSourcePath(filepath);
  return true;
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

      AccelEngine::World &physicsWorld = m_Scene->GetPhysicsWorld();
      AccelEngine::PhysicsSettings physics;
      AccelEngine::Vector2 gravity(0.0f, -980.0f);
      if (auto p = settings["physics"]) {
        float gx = gravity.x, gy = gravity.y;
        if (p["gravity"] && ReadVec2(p["gravity"], gx, gy))
          gravity = {gx, gy};
        if (p["substeps"])
          TryGet(p, "substeps", physics.substeps);
        if (p["velocity_iterations"])
          TryGet(p, "velocity_iterations", physics.velocityIterations);
        if (p["position_iterations"])
          TryGet(p, "position_iterations", physics.positionIterations);
        if (p["enable_sleep"])
          TryGet(p, "enable_sleep", physics.enableSleep);
        physics.substeps = std::clamp(physics.substeps, 1, 64);
        physics.velocityIterations = std::clamp(physics.velocityIterations, 1, 64);
        physics.positionIterations = std::clamp(physics.positionIterations, 0, 64);
      }
      physicsWorld.settings = physics;
      physicsWorld.gravity = gravity;
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
      std::string bus;
      if (a["bus"] && TryGet(a, "bus", bus))
        audio.bus = bus == "music" ? 1 : bus == "ui" ? 2 : 0;
    });

    component("Follow", [&](const YAML::Node &f) {
      auto &follow = entity.AddComponent<FollowComponent>();
      if (f["target"]) TryGet(f, "target", follow.target);
      ReadVec2(f["offset"], follow.offset.x, follow.offset.y);
      if (f["smoothing"]) TryGet(f, "smoothing", follow.smoothing);
      if (f["stop_distance"]) TryGet(f, "stop_distance", follow.stopDistance);
      if (f["follow_x"]) TryGet(f, "follow_x", follow.followX);
      if (f["follow_y"]) TryGet(f, "follow_y", follow.followY);
      if (f["enabled"]) TryGet(f, "enabled", follow.enabled);
    });

    component("Button", [&](const YAML::Node &b) {
      auto &button = entity.AddComponent<ButtonComponent>();
      if (b["enabled"]) TryGet(b, "enabled", button.enabled);
      std::string which;
      if (b["button"] && TryGet(b, "button", which))
        button.button = which == "right" ? 1 : which == "middle" ? 2 : 0;
      if (b["hover_scale"]) TryGet(b, "hover_scale", button.hoverScale);
      if (b["press_scale"]) TryGet(b, "press_scale", button.pressScale);
      if (b["hover_sound"]) TryGet(b, "hover_sound", button.hoverSound);
      if (b["click_sound"]) TryGet(b, "click_sound", button.clickSound);
      if (b["volume"]) TryGet(b, "volume", button.volume);
      if (b["padding"]) TryGet(b, "padding", button.padding);
    });

    component("ShaderCanvas", [&](const YAML::Node &c) {
      auto &canvas = entity.AddComponent<ShaderCanvasComponent>();
      if (c["shader"]) TryGet(c, "shader", canvas.shader);
      std::string scope;
      if (c["scope"] && TryGet(c, "scope", scope))
        canvas.scope = scope == "children" ? 1 : scope == "z_range" ? 2 : 0;
      ReadVec2(c["z_range"], canvas.zMin, canvas.zMax);
      if (c["order"]) TryGet(c, "order", canvas.order);
      if (c["resolution_scale"]) TryGet(c, "resolution_scale", canvas.resolutionScale);
      if (c["progress"]) TryGet(c, "progress", canvas.progress);
      if (c["enabled"]) TryGet(c, "enabled", canvas.enabled);
      if (c["limit_area"]) TryGet(c, "limit_area", canvas.limitArea);
      ReadVec2(c["area_size"], canvas.areaSize.x, canvas.areaSize.y);
      if (auto list = c["uniforms"]; list && list.IsSequence()) {
        for (const auto &item : list) {
          if (!item.IsMap() || !item["name"])
            continue;
          ShaderUniformValue uniform;
          uniform.name = item["name"].as<std::string>();
          const std::string type = item["type"] ? item["type"].as<std::string>() : "float";
          uniform.type = type == "vec2" ? 1 : type == "vec3" ? 2 : type == "vec4" ? 3
                       : type == "color" ? 4 : type == "texture" ? 5
                       : type == "int" ? 6 : type == "bool" ? 7 : 0;
          if (uniform.type == 5) {
            if (item["texture"]) {
              uniform.texturePath = item["texture"].as<std::string>();
              if (!uniform.texturePath.empty())
                uniform.texture = TextureManager::Get().LoadTexture(uniform.texturePath, uniform.texturePath);
            }
          } else if (auto v = item["value"]; v && v.IsSequence()) {
            for (size_t i = 0; i < 4 && i < v.size(); ++i)
              uniform.value[i] = v[i].as<float>();
          }
          canvas.uniforms.push_back(std::move(uniform));
        }
      }
    });

    component("PrefabInstance", [&](const YAML::Node &p) {
      auto &link = entity.AddComponent<PrefabInstanceComponent>();
      if (p["path"]) TryGet(p, "path", link.path);
    });

    component("Canvas", [&](const YAML::Node &c) {
      auto &canvas = entity.AddComponent<CanvasComponent>();
      ReadVec2(c["reference_size"], canvas.referenceSize.x, canvas.referenceSize.y);
      std::string mode;
      if (c["scale_mode"] && TryGet(c, "scale_mode", mode))
        canvas.scaleMode = mode == "constant" ? 0 : 1;
      if (c["match"]) TryGet(c, "match", canvas.match);
      if (c["safe_area"]) TryGet(c, "safe_area", canvas.safeArea);
      if (c["sort_order"]) TryGet(c, "sort_order", canvas.sortOrder);
      if (c["visible"]) TryGet(c, "visible", canvas.visible);
      canvas.size = canvas.referenceSize;
    });

    component("UIRect", [&](const YAML::Node &r) {
      auto &rect = entity.AddComponent<UIRectComponent>();
      ReadVec2(r["anchor_min"], rect.anchorMin.x, rect.anchorMin.y);
      ReadVec2(r["anchor_max"], rect.anchorMax.x, rect.anchorMax.y);
      ReadVec2(r["pivot"], rect.pivot.x, rect.pivot.y);
      ReadVec2(r["position"], rect.position.x, rect.position.y);
      ReadVec2(r["size"], rect.size.x, rect.size.y);
      if (auto m = r["margin"]; m && m.IsSequence() && m.size() >= 4)
        rect.margin = {m[0].as<float>(), m[1].as<float>(), m[2].as<float>(), m[3].as<float>()};
      if (r["visible"]) TryGet(r, "visible", rect.visible);
      if (r["blocks_input"]) TryGet(r, "blocks_input", rect.blocksInput);
    });

    component("UIPanel", [&](const YAML::Node &p) {
      auto &panel = entity.AddComponent<UIPanelComponent>();
      ReadColor(p["color"], panel.color);
      if (p["corner_radius"]) TryGet(p, "corner_radius", panel.cornerRadius);
      ReadColor(p["border_color"], panel.borderColor);
      if (p["border_width"]) TryGet(p, "border_width", panel.borderWidth);
    });

    component("UILabel", [&](const YAML::Node &l) {
      auto &label = entity.AddComponent<UILabelComponent>();
      if (l["text"]) TryGet(l, "text", label.text);
      if (l["size"]) TryGet(l, "size", label.size);
      ReadColor(l["color"], label.color);
      if (l["align"]) TryGet(l, "align", label.align);
      if (l["vertical_align"]) TryGet(l, "vertical_align", label.verticalAlign);
      if (l["font"]) TryGet(l, "font", label.fontPath);
      if (l["shadow"]) TryGet(l, "shadow", label.shadow);
    });

    component("UIImage", [&](const YAML::Node &i) {
      auto &image = entity.AddComponent<UIImageComponent>();
      if (i["texture"]) {
        std::string path = i["texture"].as<std::string>();
        if (!path.empty())
          image.texture = TextureManager::Get().LoadTexture(path, path);
        if (image.texture && i["filter"])
          image.texture->SetFilter(i["filter"].as<std::string>() == "linear"
                                       ? TextureFilter::Linear
                                       : TextureFilter::Nearest);
      }
      ReadColor(i["tint"], image.tint);
      if (i["preserve_aspect"]) TryGet(i, "preserve_aspect", image.preserveAspect);
      if (auto r = i["region"]; r && r.IsSequence() && r.size() >= 4) {
        image.useRegion = true;
        image.region = {r[0].as<float>(), r[1].as<float>(), r[2].as<float>(), r[3].as<float>()};
      }
    });

    component("UIButton", [&](const YAML::Node &b) {
      auto &button = entity.AddComponent<UIButtonComponent>();
      if (b["enabled"]) TryGet(b, "enabled", button.enabled);
      if (b["focusable"]) TryGet(b, "focusable", button.focusable);
      ReadColor(b["hover_tint"], button.hoverTint);
      ReadColor(b["press_tint"], button.pressTint);
      ReadColor(b["disabled_tint"], button.disabledTint);
      if (b["hover_scale"]) TryGet(b, "hover_scale", button.hoverScale);
      if (b["press_scale"]) TryGet(b, "press_scale", button.pressScale);
      if (b["hover_sound"]) TryGet(b, "hover_sound", button.hoverSound);
      if (b["click_sound"]) TryGet(b, "click_sound", button.clickSound);
      if (b["volume"]) TryGet(b, "volume", button.volume);
    });

    component("UILayout", [&](const YAML::Node &l) {
      auto &layout = entity.AddComponent<UILayoutComponent>();
      std::string direction;
      if (l["direction"] && TryGet(l, "direction", direction))
        layout.direction = direction == "row" ? 0 : 1;
      if (l["spacing"]) TryGet(l, "spacing", layout.spacing);
      if (auto p = l["padding"]; p && p.IsSequence() && p.size() >= 4)
        layout.padding = {p[0].as<float>(), p[1].as<float>(), p[2].as<float>(), p[3].as<float>()};
      if (l["justify"]) TryGet(l, "justify", layout.justify);
      if (l["align"]) TryGet(l, "align", layout.align);
      if (l["stretch"]) TryGet(l, "stretch", layout.stretch);
      if (l["enabled"]) TryGet(l, "enabled", layout.enabled);
    });

    component("Teleporter", [&](const YAML::Node &t) {
      auto &portal = entity.AddComponent<TeleporterComponent>();
      if (t["target"]) TryGet(t, "target", portal.target);
      ReadVec2(t["offset"], portal.offset.x, portal.offset.y);
      if (t["sound"]) TryGet(t, "sound", portal.sound);
      if (t["keep_velocity"]) TryGet(t, "keep_velocity", portal.keepVelocity);
      if (t["enabled"]) TryGet(t, "enabled", portal.enabled);
    });

    component("CameraZone", [&](const YAML::Node &z) {
      auto &zone = entity.AddComponent<CameraZoneComponent>();
      if (z["zoom"]) TryGet(z, "zoom", zone.zoom);
      if (z["speed"]) TryGet(z, "speed", zone.speed);
      if (z["set_bounds"]) TryGet(z, "set_bounds", zone.setBounds);
      ReadVec2(z["bounds_min"], zone.boundsMin.x, zone.boundsMin.y);
      ReadVec2(z["bounds_max"], zone.boundsMax.x, zone.boundsMax.y);
      if (z["restore_on_exit"]) TryGet(z, "restore_on_exit", zone.restoreOnExit);
      if (z["enabled"]) TryGet(z, "enabled", zone.enabled);
    });

    component("WorldBounds", [&](const YAML::Node &w) {
      auto &bounds = entity.AddComponent<WorldBoundsComponent>();
      ReadVec2(w["min"], bounds.min.x, bounds.min.y);
      ReadVec2(w["max"], bounds.max.x, bounds.max.y);
      std::string mode;
      if (w["mode"] && TryGet(w, "mode", mode))
        bounds.mode = mode == "wrap"      ? BoundsMode::Wrap
                      : mode == "bounce"  ? BoundsMode::Bounce
                      : mode == "destroy" ? BoundsMode::Destroy
                                          : BoundsMode::Clamp;
      if (w["margin"]) TryGet(w, "margin", bounds.margin);
      if (w["enabled"]) TryGet(w, "enabled", bounds.enabled);
    });

    component("Pickable", [&](const YAML::Node &p) {
      auto &pickable = entity.AddComponent<PickableComponent>();
      if (p["enabled"]) TryGet(p, "enabled", pickable.enabled);
      std::string button;
      if (p["button"] && TryGet(p, "button", button))
        pickable.button = button == "right" ? 1 : button == "middle" ? 2 : 0;
      if (p["smoothing"]) TryGet(p, "smoothing", pickable.smoothing);
      if (p["throw_strength"]) TryGet(p, "throw_strength", pickable.throwStrength);
      if (p["lift_scale"]) TryGet(p, "lift_scale", pickable.liftScale);
      if (p["bring_to_front"]) TryGet(p, "bring_to_front", pickable.bringToFront);
      if (p["keep_offset"]) TryGet(p, "keep_offset", pickable.keepOffset);
      if (p["padding"]) TryGet(p, "padding", pickable.padding);
    });

    component("Pickup", [&](const YAML::Node &p) {
      auto &pickup = entity.AddComponent<PickupComponent>();
      if (p["key"]) TryGet(p, "key", pickup.key);
      if (p["amount"]) TryGet(p, "amount", pickup.amount);
      if (p["sound"]) TryGet(p, "sound", pickup.sound);
      if (p["volume"]) TryGet(p, "volume", pickup.volume);
      if (p["destroy_on_pickup"]) TryGet(p, "destroy_on_pickup", pickup.destroyOnPickup);
      if (p["enabled"]) TryGet(p, "enabled", pickup.enabled);
    });

    component("Checkpoint", [&](const YAML::Node &c) {
      auto &checkpoint = entity.AddComponent<CheckpointComponent>();
      ReadVec2(c["offset"], checkpoint.offset.x, checkpoint.offset.y);
      if (c["sound"]) TryGet(c, "sound", checkpoint.sound);
      if (c["enabled"]) TryGet(c, "enabled", checkpoint.enabled);
    });

    component("Respawn", [&](const YAML::Node &r) {
      auto &respawn = entity.AddComponent<RespawnComponent>();
      if (r["delay"]) TryGet(r, "delay", respawn.delay);
      if (r["use_kill_y"]) TryGet(r, "use_kill_y", respawn.useKillY);
      if (r["kill_y"]) TryGet(r, "kill_y", respawn.killY);
      if (r["restore_health"]) TryGet(r, "restore_health", respawn.restoreHealth);
    });

    component("Music", [&](const YAML::Node &m) {
      auto &music = entity.AddComponent<MusicComponent>();
      if (auto tracks = m["tracks"]; tracks && tracks.IsSequence())
        for (const auto &track : tracks)
          music.tracks.push_back(track.as<std::string>());
      if (m["volume"]) TryGet(m, "volume", music.volume);
      if (m["fade_in"]) TryGet(m, "fade_in", music.fadeIn);
      if (m["fade_out"]) TryGet(m, "fade_out", music.fadeOut);
      if (m["shuffle"]) TryGet(m, "shuffle", music.shuffle);
      if (m["loop_playlist"]) TryGet(m, "loop_playlist", music.loopPlaylist);
      if (m["play_on_start"]) TryGet(m, "play_on_start", music.playOnStart);
      if (m["on_trigger"]) TryGet(m, "on_trigger", music.onTrigger);
      if (m["stop_on_exit"]) TryGet(m, "stop_on_exit", music.stopOnExit);
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
      using AccelEngine::ShapeType;
      std::string shape;
      if (!TryGet(p, "shape", shape))
        return;

      float mass = 1.0f;
      if (p["mass"]) {
        TryGet(p, "mass", mass);
      } else {
        float inverseMass = 1.0f;
        if (!TryGet(p, "inverse_mass", inverseMass))
          return;
        mass = inverseMass > 0.0f ? 1.0f / inverseMass : 0.0f;
      }
      if (!std::isfinite(mass) || mass < 0.0f)
        mass = 0.0f;

      PhysicsBodies::Options options;
      options.Mass = mass;

      std::unique_ptr<AccelEngine::RigidBody> body;
      if (shape == "CIRCLE") {
        float radius = 0.0f;
        if (!TryGet(p, "radius", radius))
          return;
        body = PhysicsBodies::Circle(radius, options);
      } else if (shape == "AABB") {
        float hx = 0.0f, hy = 0.0f;
        if (!ReadVec2(p["half_size"], hx, hy))
          return;
        body = PhysicsBodies::Box({hx, hy}, options);
      } else if (shape == "POLYGON" || shape == "CAPSULE" || shape == "COMPOUND") {
        std::vector<AccelEngine::ConvexPart> parts;
        if (auto list = p["parts"]; list && list.IsSequence()) {
          for (const auto &partNode : list) {
            std::vector<glm::vec2> points;
            if (auto vertices = partNode["vertices"]; vertices && vertices.IsSequence()) {
              for (const auto &vertex : vertices) {
                float x = 0.0f, y = 0.0f;
                if (ReadVec2(vertex, x, y))
                  points.emplace_back(x, y);
              }
            }
            float radius = 0.0f;
            if (partNode["radius"])
              TryGet(partNode, "radius", radius);
            if (!points.empty())
              parts.push_back(PhysicsBodies::MakePart(points, radius));
          }
        }
        const ShapeType type = shape == "POLYGON" ? ShapeType::POLYGON : shape == "CAPSULE" ? ShapeType::CAPSULE : ShapeType::COMPOUND;
        body = PhysicsBodies::FromParts(type, std::move(parts), options, false);
      }
      if (!body)
        return;

      if (p["angular_velocity"])
        TryGet(p, "angular_velocity", body->rotation);
      else if (p["rotation"])
        TryGet(p, "rotation", body->rotation);

      float vx = 0.0f, vy = 0.0f;
      if (ReadVec2(p["velocity"], vx, vy))
        body->velocity = {vx, vy};

      TryGet(p, "restitution", body->restitution);
      TryGet(p, "static_friction", body->staticFriction);
      TryGet(p, "dynamic_friction", body->dynamicFriction);
      TryGet(p, "linear_damping", body->linearDamping);
      TryGet(p, "angular_damping", body->angularDamping);
      TryGet(p, "lock_rotation", body->lockRotation);
      TryGet(p, "lock_position", body->lockPosition);
      TryGet(p, "ignore_gravity", body->ignoreGravity);
      TryGet(p, "can_sleep", body->canSleep);
      int category = body->categoryBits, mask = body->maskBits, group = body->groupIndex;
      TryGet(p, "category", category);
      TryGet(p, "mask", mask);
      TryGet(p, "group", group);
      body->categoryBits = static_cast<uint16_t>(category);
      body->maskBits = static_cast<uint16_t>(mask);
      body->groupIndex = static_cast<int16_t>(group);
      TryGet(p, "sensor", body->isSensor);
      TryGet(p, "one_way", body->oneWay);
      float nx = body->oneWayNormal.x, ny = body->oneWayNormal.y;
      if (ReadVec2(p["one_way_normal"], nx, ny))
        body->oneWayNormal = {nx, ny};

      PhysicsBodies::Refresh(*body);
      m_Scene->AddBody(entity.GetHandle(), std::move(body));
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
