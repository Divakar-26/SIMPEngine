#include "EditorCommands.h"
#include "EditorHistory.h"
#include <Engine/Core/Log.h>
#include <Engine/Rendering/TextureManager.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/SceneSerializer.h>
#include <Engine/Scene/Loaders/TiledLoader.h>
#include <Engine/Rendering/Tileset.h>
#include <Engine/Particles/ParticleEmitter.h>
#include <Engine/Particles/ParticleSerializer.h>
#include <Engine/Math/EntityUtility.h>
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

SIMPEngine::Entity EditorCommands::CreateParticleEmitter(EditorContext &ctx, const SIMPEngine::ParticleEffect &effect, const glm::vec2 &position)
{
    auto entity = CreateEntity(ctx, effect.Name.empty() ? "Particles" : effect.Name);
    auto &transform = entity.GetComponent<TransformComponent>();
    transform.position = position;
    transform.dirty = true;
    auto &emitter = entity.AddComponent<ParticleEmitterComponent>();
    emitter.effect = effect;
    ctx.Scene->UpdateTransforms();
    ctx.Select(entity);
    return entity;
}

void EditorCommands::ApplyParticleEffect(EditorContext &ctx, SIMPEngine::Entity entity, const SIMPEngine::ParticleEffect &effect)
{
    if (!entity || !entity.HasComponent<ParticleEmitterComponent>())
        return;
    RecordEdit(ctx);
    auto &emitter = entity.GetComponent<ParticleEmitterComponent>();
    emitter.effect = effect;
    emitter.playing = true;
    SIMPEngine::ParticleEmitter::Restart(emitter.state);
}

bool EditorCommands::SaveParticleEffect(const SIMPEngine::ParticleEffect &effect, std::string &outPath)
{
    std::string name = effect.Name.empty() ? "Particles" : effect.Name;
    std::string file;
    for (char c : name)
        file += (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') ? c : '_';
    outPath = "assets://particles/" + file + SIMPEngine::ParticleSerializer::Extension;
    bool ok = SIMPEngine::ParticleSerializer::SaveFile(outPath, effect);
    if (ok)
        CORE_INFO("Saved particle effect {}", outPath);
    return ok;
}

SIMPEngine::Entity EditorCommands::CreateLight(EditorContext &ctx, bool spot, const glm::vec2 &position)
{
    auto entity = CreateEntity(ctx, spot ? "Spot Light" : "Point Light");
    auto &transform = entity.GetComponent<TransformComponent>();
    transform.position = position;
    transform.rotation = spot ? -90.0f : 0.0f;
    transform.dirty = true;
    auto &light = entity.AddComponent<LightComponent>();
    if (spot)
    {
        light.type = LightType::Spot;
        light.radius = 360.0f;
        light.falloff = 1.2f;
    }
    if (!ctx.Scene->GetLighting().Enabled)
    {
        ctx.Scene->GetLighting().Enabled = true;
        CORE_INFO("Scene lighting enabled (ambient can be changed in the Scene inspector)");
    }
    ctx.Scene->UpdateTransforms();
    ctx.Select(entity);
    return entity;
}

SIMPEngine::Entity EditorCommands::CreateTilemap(EditorContext &ctx)
{
    auto entity = CreateEntity(ctx, "Tilemap");
    auto &tilemap = entity.AddComponent<TilemapComponent>();
    tilemap.tileSize = 32.0f;
    ctx.Select(entity);
    ctx.Tiles.Enabled = true;
    ctx.Tiles.FocusPalette = true;
    return entity;
}

bool EditorCommands::AssignTileset(EditorContext &ctx, SIMPEngine::Entity entity, const std::string &path)
{
    if (!entity || !entity.HasComponent<TilemapComponent>())
        return false;

    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c)
                   { return static_cast<char>(std::tolower(c)); });

    std::shared_ptr<SIMPEngine::Tileset> tileset;
    auto &tilemap = entity.GetComponent<TilemapComponent>();
    if (ext == ".json" || ext == ".tsj")
    {
        tileset = SIMPEngine::TiledLoader::LoadTileset(path);
    }
    else if (auto texture = LoadTexture(path))
    {
        int tw = tilemap.tileset ? tilemap.tileset->GetTileWidth() : std::max(1, static_cast<int>(tilemap.tileSize));
        int th = tilemap.tileset ? tilemap.tileset->GetTileHeight() : std::max(1, static_cast<int>(tilemap.tileSize));
        tw = std::min(tw, texture->GetWidth());
        th = std::min(th, texture->GetHeight());
        tileset = std::make_shared<SIMPEngine::Tileset>(texture, tw, th);
    }

    if (!tileset)
    {
        CORE_ERROR("Could not use '{}' as a tileset", path);
        return false;
    }

    RecordEdit(ctx);
    tilemap.tileset = tileset;
    ctx.Tiles.SetSingle(0);
    return true;
}

SIMPEngine::Entity EditorCommands::ImportTiledMap(EditorContext &ctx, const std::string &path)
{
    RecordEdit(ctx);
    auto created = SIMPEngine::TiledLoader::Import(path, *ctx.Scene);
    if (created.empty())
        return {};
    SIMPEngine::Entity root(created.front(), ctx.Scene);
    ctx.Select(root);
    return root;
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

void EditorCommands::DeleteSelection(EditorContext &ctx)
{
    std::vector<entt::entity> roots = ctx.SelectedRoots();
    for (entt::entity handle : roots)
        DeleteEntity(ctx, SIMPEngine::Entity(handle, ctx.Scene));
    ctx.ClearSelection();
}

void EditorCommands::DuplicateSelection(EditorContext &ctx)
{
    std::vector<entt::entity> roots = ctx.SelectedRoots();
    std::vector<entt::entity> created;
    for (entt::entity handle : roots)
    {
        SIMPEngine::Entity copy = DuplicateEntity(ctx, SIMPEngine::Entity(handle, ctx.Scene));
        if (copy)
            created.push_back(copy.GetHandle());
    }
    ctx.SetSelection(created);
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

namespace
{
    void Detach(entt::registry &registry, entt::entity child)
    {
        auto *hierarchy = registry.try_get<HierarchyComponent>(child);
        if (!hierarchy)
            return;
        if (hierarchy->parent != entt::null && registry.valid(hierarchy->parent))
        {
            if (auto *old = registry.try_get<HierarchyComponent>(hierarchy->parent))
                old->children.erase(std::remove(old->children.begin(), old->children.end(), child), old->children.end());
        }
        hierarchy->parent = entt::null;
    }

    entt::entity ParentOf(entt::registry &registry, entt::entity entity)
    {
        auto *hierarchy = registry.try_get<HierarchyComponent>(entity);
        return hierarchy && registry.valid(hierarchy->parent) ? hierarchy->parent : entt::null;
    }

    bool HasAncestorIn(entt::registry &registry, entt::entity entity, const std::vector<entt::entity> &set)
    {
        for (entt::entity a = ParentOf(registry, entity); a != entt::null; a = ParentOf(registry, a))
            if (std::find(set.begin(), set.end(), a) != set.end())
                return true;
        return false;
    }
}

bool EditorCommands::IsGroup(SIMPEngine::Entity entity)
{
    if (!entity)
        return false;
    auto &registry = entity.GetScene()->GetRegistry();
    entt::entity handle = entity.GetHandle();
    auto *hierarchy = registry.try_get<HierarchyComponent>(handle);
    if (!hierarchy || hierarchy->children.empty())
        return false;
    return !registry.any_of<SpriteComponent, RenderComponent, TextComponent, TilemapComponent, AnimatedSpriteComponent, ParticleEmitterComponent,
                            LightComponent, AudioSourceComponent, CameraComponent, PhysicsComponent, CollisionComponent, ScriptComponent, GlowComponent>(handle);
}

SIMPEngine::Entity EditorCommands::GroupSelection(EditorContext &ctx, const std::string &name)
{
    auto &registry = ctx.Scene->GetRegistry();
    std::vector<entt::entity> selected;
    for (entt::entity handle : ctx.SelectedHandles())
        if (registry.valid(handle))
            selected.push_back(handle);

    std::vector<entt::entity> tops;
    for (entt::entity handle : selected)
        if (!HasAncestorIn(registry, handle, selected))
            tops.push_back(handle);
    if (tops.empty())
        return {};

    RecordEdit(ctx);
    ctx.Scene->UpdateTransforms();

    entt::entity commonParent = ParentOf(registry, tops.front());
    bool shared = std::all_of(tops.begin(), tops.end(), [&](entt::entity e) { return ParentOf(registry, e) == commonParent; });

    auto group = ctx.Scene->CreateEntity(name);
    if (shared && commonParent != entt::null)
        ctx.Scene->SetParent(group.GetHandle(), commonParent);

    for (entt::entity handle : tops)
    {
        auto &transform = registry.get<TransformComponent>(handle);
        if (!shared)
        {
            auto world = SIMPEngine::DecomposeWorld(transform);
            transform.position = world.Position;
            transform.rotation = world.Rotation;
            transform.scale = world.Scale;
        }
        Detach(registry, handle);
        ctx.Scene->SetParent(handle, group.GetHandle());
        transform.dirty = true;
    }

    ctx.Scene->UpdateTransforms();
    ctx.Select(group);
    return group;
}

void EditorCommands::Ungroup(EditorContext &ctx, SIMPEngine::Entity group)
{
    if (!group || !group.HasComponent<HierarchyComponent>())
        return;
    auto &registry = ctx.Scene->GetRegistry();
    entt::entity handle = group.GetHandle();
    std::vector<entt::entity> children = registry.get<HierarchyComponent>(handle).children;
    if (children.empty())
        return;

    RecordEdit(ctx);
    entt::entity parent = ParentOf(registry, handle);
    const auto &groupTransform = registry.get<TransformComponent>(handle);
    glm::mat4 local = groupTransform.GetLocalTransform();
    float rotation = groupTransform.rotation;
    glm::vec2 scale = groupTransform.scale;

    std::vector<entt::entity> moved;
    for (entt::entity child : children)
    {
        if (!registry.valid(child))
            continue;
        auto &transform = registry.get<TransformComponent>(child);
        glm::vec4 p = local * glm::vec4(transform.position, 0.0f, 1.0f);
        transform.position = {p.x, p.y};
        transform.rotation += rotation;
        transform.scale *= scale;
        transform.dirty = true;
        Detach(registry, child);
        if (parent != entt::null)
            ctx.Scene->SetParent(child, parent);
        moved.push_back(child);
    }

    ctx.Scene->DestroyEntity(group);
    ctx.Scene->UpdateTransforms();
    ctx.SetSelection(moved);
}
