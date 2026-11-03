#include "EditorCommands.h"
#include "EditorHistory.h"
#include <Engine/Core/Log.h>
#include <Engine/Rendering/TextureManager.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/SceneSerializer.h>
#include <Engine/Scene/ScriptableEntity.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <functional>

namespace
{
    void CollectSubtree(entt::registry &registry, entt::entity root, std::vector<entt::entity> &out)
    {
        out.push_back(root);
        if (auto *h = registry.try_get<HierarchyComponent>(root))
            for (auto child : h->children)
                if (registry.valid(child) && std::find(out.begin(), out.end(), child) == out.end())
                    CollectSubtree(registry, child, out);
    }

    std::string NextName(SIMPEngine::Scene &scene, const std::string &name)
    {
        std::string base = name;
        while (!base.empty() && std::isdigit(static_cast<unsigned char>(base.back())))
            base.pop_back();
        if (base.empty())
            base = name;
        else if (base.size() != name.size() && base.back() == ' ')
            base.pop_back();

        for (int i = 2; i < 10000; ++i)
        {
            std::string candidate = base + " " + std::to_string(i);
            if (!scene.GetEntityByName(candidate))
                return candidate;
        }
        return name;
    }
}

void EditorCommands::RecordEdit(EditorContext &ctx)
{
    if (ctx.History)
        ctx.History->BeginEdit();
}

SIMPEngine::Entity EditorCommands::CreateEntity(EditorContext &ctx, const std::string &name)
{
    RecordEdit(ctx);
    auto entity = ctx.Scene->CreateEntity(name);
    return entity;
}

SIMPEngine::Entity EditorCommands::CreateChild(EditorContext &ctx, SIMPEngine::Entity parent, const std::string &name)
{
    RecordEdit(ctx);
    auto entity = ctx.Scene->CreateEntity(name);
    if (parent)
        ReparentEntity(ctx, entity, parent);
    return entity;
}

std::shared_ptr<SIMPEngine::Texture> EditorCommands::LoadTexture(const std::string &vpath)
{
    auto texture = SIMPEngine::TextureManager::Get().LoadTexture(vpath, vpath);
    if (texture)
        texture->SetVFSPath(vpath);
    else
        CORE_ERROR("Could not load texture {}", vpath);
    return texture;
}

SIMPEngine::Entity EditorCommands::CreateSprite(EditorContext &ctx, const std::string &textureVPath, const glm::vec2 &position)
{
    auto texture = LoadTexture(textureVPath);
    if (!texture)
        return {};

    std::string name = std::filesystem::path(textureVPath).stem().string();
    auto entity = CreateEntity(ctx, name.empty() ? "Sprite" : name);
    auto &tc = entity.GetComponent<TransformComponent>();
    tc.position = position;

    auto &sprite = entity.AddComponent<SpriteComponent>();
    sprite.texture = texture;
    sprite.width = static_cast<float>(texture->GetWidth());
    sprite.height = static_cast<float>(texture->GetHeight());
    return entity;
}

void EditorCommands::DeleteEntity(
    EditorContext &ctx,
    SIMPEngine::Entity entity)
{
    auto &registry = ctx.Scene->GetRegistry();
    if (!entity || !registry.valid(entity.GetHandle()))
        return;

    RecordEdit(ctx);

    std::vector<entt::entity> subtree;
    CollectSubtree(registry, entity.GetHandle(), subtree);

    for (auto it = subtree.rbegin(); it != subtree.rend(); ++it)
    {
        if (registry.valid(*it))
            registry.destroy(*it);
    }

    if (ctx.SelectedEntity && !registry.valid(ctx.SelectedEntity.GetHandle()))
        ctx.SelectedEntity = {};
    if (ctx.HoveredEntity && !registry.valid(ctx.HoveredEntity.GetHandle()))
        ctx.HoveredEntity = {};
}

SIMPEngine::Entity EditorCommands::DuplicateEntity(EditorContext &ctx, SIMPEngine::Entity entity)
{
    auto &registry = ctx.Scene->GetRegistry();
    if (!entity || !registry.valid(entity.GetHandle()))
        return {};

    RecordEdit(ctx);

    std::vector<entt::entity> subtree;
    CollectSubtree(registry, entity.GetHandle(), subtree);

    SIMPEngine::SceneSerializer serializer(ctx.Scene);
    auto created = serializer.DeserializeEntities(serializer.SerializeEntities(subtree));
    if (created.empty())
        return {};

    SIMPEngine::Entity root(created.front(), ctx.Scene);
    auto &tag = root.GetComponent<TagComponent>().Tag;
    tag = NextName(*ctx.Scene, tag);

    if (auto *h = registry.try_get<HierarchyComponent>(entity.GetHandle()); h && h->parent != entt::null && registry.valid(h->parent))
        ReparentEntity(ctx, root, SIMPEngine::Entity(h->parent, ctx.Scene));

    for (size_t i = 0; i < subtree.size() && i < created.size(); ++i)
    {
        if (auto *sc = registry.try_get<ScriptComponent>(subtree[i]); sc && sc->InstantiateScript)
            registry.emplace_or_replace<ScriptComponent>(created[i]).InstantiateScript = sc->InstantiateScript;
    }

    ctx.Scene->MarkPhysicsDirty();
    return root;
}

void EditorCommands::RenameEntity(
    EditorContext &ctx,
    SIMPEngine::Entity entity,
    const std::string &name)
{
    if (!entity || !entity.HasComponent<TagComponent>())
        return;
    auto &tag = entity.GetComponent<TagComponent>().Tag;
    if (tag == name)
        return;
    RecordEdit(ctx);
    tag = name;
}

bool EditorCommands::ReparentEntity(
    EditorContext &ctx,
    SIMPEngine::Entity child,
    SIMPEngine::Entity parent)
{
    auto &registry = ctx.Scene->GetRegistry();
    entt::entity c = child.GetHandle();
    entt::entity p = parent ? parent.GetHandle() : entt::null;

    if (!registry.valid(c) || c == p)
        return false;

    for (entt::entity a = p; a != entt::null && registry.valid(a);)
    {
        if (a == c)
            return false;
        auto *h = registry.try_get<HierarchyComponent>(a);
        a = h ? h->parent : entt::null;
    }

    auto *existing = registry.try_get<HierarchyComponent>(c);
    if (existing && existing->parent == p)
        return false;
    if (!existing && p == entt::null)
        return false;

    RecordEdit(ctx);

    auto &ch = registry.get_or_emplace<HierarchyComponent>(c);
    if (ch.parent != entt::null && registry.valid(ch.parent))
    {
        if (auto *old = registry.try_get<HierarchyComponent>(ch.parent))
            old->children.erase(std::remove(old->children.begin(), old->children.end(), c), old->children.end());
    }
    ch.parent = entt::null;

    if (p != entt::null)
        ctx.Scene->SetParent(c, p);

    if (auto *tc = registry.try_get<TransformComponent>(c))
        tc->dirty = true;

    ctx.Scene->UpdateTransforms();
    return true;
}
