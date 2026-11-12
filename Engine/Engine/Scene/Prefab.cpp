#include <Engine/Scene/Prefab.h>

#include <Engine/Core/FileSystem.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneSerializer.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <unordered_map>

namespace SIMPEngine::Prefab
{
    namespace
    {
        struct Cached
        {
            std::filesystem::file_time_type Time{};
            std::string Yaml;
        };

        std::unordered_map<std::string, Cached> s_Cache;

        void CollectSubtree(entt::registry &registry, entt::entity root, std::vector<entt::entity> &out, int depth = 0)
        {
            if (!registry.valid(root) || depth > 64)
                return;
            out.push_back(root);
            if (const auto *hierarchy = registry.try_get<HierarchyComponent>(root))
                for (entt::entity child : hierarchy->children)
                    CollectSubtree(registry, child, out, depth + 1);
        }
    }

    bool IsPrefabPath(const std::string &path)
    {
        if (path.size() < 7)
            return false;
        std::string tail = path.substr(path.size() - 7);
        std::transform(tail.begin(), tail.end(), tail.begin(), [](unsigned char c)
                       { return static_cast<char>(std::tolower(c)); });
        return tail == ".prefab";
    }

    std::optional<std::string> LoadYaml(const std::string &path)
    {
        const std::optional<std::string> real = VFS::Resolve(path);
        if (!real)
        {
            CORE_WARN("Prefab: cannot resolve '{}'", path);
            return std::nullopt;
        }

        std::error_code ec;
        const auto time = std::filesystem::last_write_time(std::filesystem::path(*real), ec);
        if (auto it = s_Cache.find(path); it != s_Cache.end() && !ec && it->second.Time == time)
            return it->second.Yaml;

        std::optional<std::string> text = FileSystem::ReadText(*real);
        if (!text || text->empty())
        {
            CORE_WARN("Prefab: cannot read '{}'", path);
            return std::nullopt;
        }
        s_Cache[path] = {ec ? std::filesystem::file_time_type{} : time, *text};
        return text;
    }

    void ClearCache()
    {
        s_Cache.clear();
    }

    bool Save(Scene &scene, entt::entity root, const std::string &path)
    {
        entt::registry &registry = scene.GetRegistry();
        if (!registry.valid(root) || !IsPrefabPath(path))
            return false;

        std::vector<entt::entity> subtree;
        CollectSubtree(registry, root, subtree);

        TransformComponent *transform = registry.try_get<TransformComponent>(root);
        glm::vec2 savedPosition{0.0f};
        if (transform)
        {
            savedPosition = transform->position;
            transform->position = {0.0f, 0.0f};
        }
        const std::string yaml = SceneSerializer(&scene).SerializeEntities(subtree);
        if (transform)
            transform->position = savedPosition;

        const std::optional<std::string> real = VFS::Resolve(path);
        if (!real || yaml.empty() || !FileSystem::WriteText(*real, yaml))
        {
            CORE_ERROR("Prefab: could not write '{}'", path);
            return false;
        }
        s_Cache.erase(path);
        return true;
    }

    entt::entity Instantiate(Scene &scene, const std::string &path, const std::optional<glm::vec2> &position, entt::entity parent)
    {
        const std::optional<std::string> yaml = LoadYaml(path);
        if (!yaml)
            return entt::null;

        entt::registry &registry = scene.GetRegistry();
        std::vector<entt::entity> created = SceneSerializer(&scene).DeserializeEntities(*yaml);
        if (created.empty())
        {
            CORE_ERROR("Prefab: '{}' has no entities", path);
            return entt::null;
        }

        const entt::entity root = created.front();
        registry.emplace_or_replace<PrefabInstanceComponent>(root, PrefabInstanceComponent{path});
        if (parent != entt::null && registry.valid(parent))
            scene.SetParent(root, parent);
        if (position)
            if (auto *transform = registry.try_get<TransformComponent>(root))
            {
                transform->position = *position;
                transform->dirty = true;
            }
        scene.MarkPhysicsDirty();
        return root;
    }

    entt::entity Refresh(Scene &scene, entt::entity instance)
    {
        entt::registry &registry = scene.GetRegistry();
        if (!registry.valid(instance))
            return entt::null;
        const auto *link = registry.try_get<PrefabInstanceComponent>(instance);
        if (!link || link->path.empty())
            return entt::null;

        const std::string path = link->path;
        glm::vec2 position{0.0f};
        float rotation = 0.0f;
        glm::vec2 scale{1.0f};
        if (const auto *transform = registry.try_get<TransformComponent>(instance))
        {
            position = transform->position;
            rotation = transform->rotation;
            scale = transform->scale;
        }
        entt::entity parent = entt::null;
        if (const auto *hierarchy = registry.try_get<HierarchyComponent>(instance))
            parent = hierarchy->parent;

        const entt::entity fresh = Instantiate(scene, path, std::nullopt, parent);
        if (fresh == entt::null)
            return entt::null;
        if (auto *transform = registry.try_get<TransformComponent>(fresh))
        {
            transform->position = position;
            transform->rotation = rotation;
            transform->scale = scale;
            transform->dirty = true;
        }

        std::vector<entt::entity> old;
        CollectSubtree(registry, instance, old);
        for (auto it = old.rbegin(); it != old.rend(); ++it)
            if (registry.valid(*it))
                registry.destroy(*it);
        scene.MarkPhysicsDirty();
        return fresh;
    }
}
