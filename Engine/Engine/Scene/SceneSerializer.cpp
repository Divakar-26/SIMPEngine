#include "SceneSerializer.h"
#include "Entity.h"
#include "Scene/Component.h"
#include <fstream>

namespace SIMPEngine
{
    bool SceneSerializer::Serialize(const std::string &filepath)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Scene" << YAML::Value << m_Scene->GetName();

        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
        m_Scene->GetRegistry().view<TagComponent>().each([&](auto entityHandle, auto &tag)
                                                         {
            Entity entity{entityHandle, m_Scene};
            out << YAML::BeginMap;

            out << YAML::Key << "Tag" << YAML::Value << tag.Tag;

            if (entity.HasComponent<TransformComponent>())
            {
                auto& tc = entity.GetComponent<TransformComponent>();
                out << YAML::Key << "Transform" << YAML::Value << YAML::Flow
                    << YAML::BeginSeq << tc.position.x << tc.position.y
                    << tc.rotation << tc.scale.x << tc.scale.y << YAML::EndSeq;
            }

            if (entity.HasComponent<SpriteComponent>())
            {
                auto& sc = entity.GetComponent<SpriteComponent>();
                out << YAML::Key << "Sprite" << YAML::Value << YAML::Flow
                    << YAML::BeginSeq << sc.width << sc.height << YAML::EndSeq;
            }

            if(entity.HasComponent<RenderComponent>()){
                auto & rc = entity.GetComponent<RenderComponent>();
                out << YAML::Key << "Render" << YAML::Value << YAML::Flow
                    << YAML::BeginSeq << (int)rc.color.r << (int)rc.color.g << (int)rc.color.b << (int)rc.color.a << rc.width << rc.height << YAML::EndSeq;
            }

            out << YAML::EndMap; });
        out << YAML::EndSeq;

        out << YAML::EndMap;

        std::ofstream fout(filepath);
        fout << out.c_str();
        return true;
    }

    bool SceneSerializer::Deserialize(const std::string &filepath)
    {
        std::ifstream stream(filepath);
        if (!stream.is_open())
            return false;

        YAML::Node data = YAML::Load(stream);
        if (!data["Scene"])
            return false;

        std::string sceneName = data["Scene"].as<std::string>();
        m_Scene->SetName(sceneName);

        auto entities = data["Entities"];
        if (entities)
        {
            for (auto entityNode : entities)
            {
                std::string tag = entityNode["Tag"].as<std::string>();
                Entity entity = m_Scene->CreateEntity(tag);

                if (auto transform = entityNode["Transform"])
                {
                    auto &tc = entity.GetComponent<TransformComponent>();
                    tc.position.x = transform[0].as<float>();
                    tc.position.y = transform[1].as<float>();
                    tc.rotation = transform[2].as<float>();
                    tc.scale.x = transform[3].as<float>();
                    tc.scale.y = transform[4].as<float>();
                }

                if (auto sprite = entityNode["Sprite"])
                {
                    auto &sc = entity.AddComponent<SpriteComponent>();
                    sc.width = sprite[0].as<float>();
                    sc.height = sprite[1].as<float>();
                }

                if (auto render = entityNode["Render"])
                {
                    auto &rc = entity.AddComponent<RenderComponent>();
                    rc.color.r = render[0].as<Uint8>();
                    rc.color.g = render[1].as<Uint8>();
                    rc.color.b = render[2].as<Uint8>();
                    rc.color.a = render[3].as<Uint8>();
                    rc.width   = render[4].as<float>();
                    rc.height  = render[5].as<float>();
                }
            }
        }
        return true;
    }
}
