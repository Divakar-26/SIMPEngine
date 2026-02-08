#include <Engine/Scene/SceneSerializer.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Component.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Rendering/TextureManager.h>

#include <fstream>

namespace SIMPEngine
{

bool SceneSerializer::Serialize(const std::string& filepath)
{
    std::string realPath = filepath;
    if (auto resolved = VFS::Resolve(filepath))
        realPath = *resolved;
    else
        CORE_WARN("Failed to resolve VFS path '{}'", filepath);

    YAML::Emitter out;
    out << YAML::BeginMap;

    out << YAML::Key << "Scene" << YAML::Value << m_Scene->GetName();
    out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

    m_Scene->GetRegistry().view<TagComponent>().each(
    [&](auto entityHandle, auto& tag)
    {
        Entity entity{entityHandle, m_Scene};

        out << YAML::BeginMap;

        // Tag
        out << YAML::Key << "Tag" << YAML::Value << tag.Tag;

        // Transform
        if (entity.HasComponent<TransformComponent>())
        {
            auto& tc = entity.GetComponent<TransformComponent>();

            out << YAML::Key << "Transform" << YAML::BeginMap;
            out << YAML::Key << "position" << YAML::Value
                << YAML::Flow << YAML::BeginSeq << tc.position.x << tc.position.y << YAML::EndSeq;
            out << YAML::Key << "rotation" << YAML::Value << tc.rotation;
            out << YAML::Key << "scale" << YAML::Value
                << YAML::Flow << YAML::BeginSeq << tc.scale.x << tc.scale.y << YAML::EndSeq;
            out << YAML::EndMap;
        }

        // Render
        if (entity.HasComponent<RenderComponent>())
        {
            auto& rc = entity.GetComponent<RenderComponent>();

            out << YAML::Key << "Render" << YAML::BeginMap;
            out << YAML::Key << "color" << YAML::Value
                << YAML::Flow << YAML::BeginSeq
                << (int)rc.color.r << (int)rc.color.g
                << (int)rc.color.b << (int)rc.color.a
                << YAML::EndSeq;
            out << YAML::Key << "size" << YAML::Value
                << YAML::Flow << YAML::BeginSeq
                << rc.width << rc.height
                << YAML::EndSeq;
            out << YAML::EndMap;
        }

        // Sprite
        if (entity.HasComponent<SpriteComponent>())
        {
            auto& sc = entity.GetComponent<SpriteComponent>();
            if (sc.texture)
            {
                out << YAML::Key << "Sprite" << YAML::BeginMap;
                out << YAML::Key << "size" << YAML::Value
                    << YAML::Flow << YAML::BeginSeq
                    << sc.width << sc.height
                    << YAML::EndSeq;
                out << YAML::Key << "texture" << YAML::Value
                    << sc.texture->GetVFSPath();
                out << YAML::EndMap;
            }
        }

        // Animation
        if (entity.HasComponent<AnimatedSpriteComponent>())
        {
            auto& ac = entity.GetComponent<AnimatedSpriteComponent>();
            auto* anim = ac.animation;

            out << YAML::Key << "Animation" << YAML::BeginMap;
            out << YAML::Key << "texture" << YAML::Value
                << anim->GetTexture()->GetVFSPath();
            out << YAML::Key << "frame_width" << YAML::Value
                << anim->GetFrameWidth();
            out << YAML::Key << "frame_height" << YAML::Value
                << anim->GetFrameHeight();
            out << YAML::Key << "frame_count" << YAML::Value
                << anim->GetFrameNumber();
            out << YAML::Key << "frame_duration" << YAML::Value
                << (float)anim->GetFrameDuration();
            out << YAML::Key << "looping" << YAML::Value
                << anim->IsLooping();
            out << YAML::EndMap;
        }

        // Collision
        if (entity.HasComponent<CollisionComponent>())
        {
            auto& cc = entity.GetComponent<CollisionComponent>();

            out << YAML::Key << "Collision" << YAML::BeginMap;
            out << YAML::Key << "size" << YAML::Value
                << YAML::Flow << YAML::BeginSeq
                << cc.width << cc.height
                << YAML::EndSeq;
            out << YAML::Key << "offset" << YAML::Value
                << YAML::Flow << YAML::BeginSeq
                << cc.offsetX << cc.offsetY
                << YAML::EndSeq;
            out << YAML::EndMap;
        }

        // Velocity
        if (entity.HasComponent<VelocityComponent>())
        {
            auto& vc = entity.GetComponent<VelocityComponent>();

            out << YAML::Key << "Velocity" << YAML::BeginMap;
            out << YAML::Key << "value" << YAML::Value
                << YAML::Flow << YAML::BeginSeq
                << vc.vx << vc.vy
                << YAML::EndSeq;
            out << YAML::EndMap;
        }

        // Camera
        if (entity.HasComponent<CameraComponent>())
        {
            auto& cam = entity.GetComponent<CameraComponent>();

            out << YAML::Key << "Camera" << YAML::BeginMap;
            out << YAML::Key << "primary" << YAML::Value << cam.primary;
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

bool SceneSerializer::Deserialize(const std::string& filepath)
{
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

    m_Scene->SetName(data["Scene"].as<std::string>());

    auto entities = data["Entities"];
    if (!entities) return true;

    for (auto entityNode : entities)
    {
        std::string tag = entityNode["Tag"].as<std::string>();
        Entity entity = m_Scene->CreateEntity(tag);

        // Transform
        if (auto t = entityNode["Transform"])
        {
            auto& tc = entity.GetComponent<TransformComponent>();
            tc.position.x = t["position"][0].as<float>();
            tc.position.y = t["position"][1].as<float>();
            tc.rotation = t["rotation"].as<float>();
            tc.scale.x = t["scale"][0].as<float>();
            tc.scale.y = t["scale"][1].as<float>();
        }

        // Sprite
        if (auto s = entityNode["Sprite"])
        {
            auto& sc = entity.AddComponent<SpriteComponent>();
            sc.width = s["size"][0].as<float>();
            sc.height = s["size"][1].as<float>();

            std::string texPath = s["texture"].as<std::string>();
            sc.texture = TextureManager::Get().LoadTexture(texPath, texPath);
        }

        // Render
        if (auto r = entityNode["Render"])
        {
            auto& rc = entity.AddComponent<RenderComponent>();

            rc.color.r = r["color"][0].as<Uint8>();
            rc.color.g = r["color"][1].as<Uint8>();
            rc.color.b = r["color"][2].as<Uint8>();
            rc.color.a = r["color"][3].as<Uint8>();

            rc.width = r["size"][0].as<float>();
            rc.height = r["size"][1].as<float>();
        }

        // Collision
        if (auto c = entityNode["Collision"])
        {
            auto& cc = entity.AddComponent<CollisionComponent>();
            cc.width = c["size"][0].as<float>();
            cc.height = c["size"][1].as<float>();
            cc.offsetX = c["offset"][0].as<float>();
            cc.offsetY = c["offset"][1].as<float>();
        }

        // Velocity
        if (auto v = entityNode["Velocity"])
        {
            auto& vc = entity.AddComponent<VelocityComponent>();
            vc.vx = v["value"][0].as<float>();
            vc.vy = v["value"][1].as<float>();
        }

        // Camera
        if (auto cam = entityNode["Camera"])
        {
            auto& cc = entity.AddComponent<CameraComponent>();
            cc.primary = cam["primary"].as<bool>();
        }

        // Animation
        if (auto a = entityNode["Animation"])
        {
            std::string path = a["texture"].as<std::string>();
            int fw = a["frame_width"].as<int>();
            int fh = a["frame_height"].as<int>();
            int fn = a["frame_count"].as<int>();
            float fd = a["frame_duration"].as<float>();
            bool loop = a["looping"].as<bool>();

            auto tex = TextureManager::Get().LoadTexture(path, path);
            if (!tex) continue;

            auto& ac = entity.AddComponent<AnimatedSpriteComponent>();
            ac.animation = new Animation(tex, fw, fh, fn, fd, loop);
        }
    }

    return true;
}

}
