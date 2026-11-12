#pragma once

#include "EditorContext.h"

#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace SIMPEngine
{
    class Texture;
    struct ParticleEffect;
}

class EditorCommands
{
public:
    static void RecordEdit(EditorContext &ctx);

    static SIMPEngine::Entity CreateEntity(
        EditorContext& ctx,
        const std::string& name);

    static SIMPEngine::Entity CreateChild(
        EditorContext& ctx,
        SIMPEngine::Entity parent,
        const std::string& name);

    static SIMPEngine::Entity CreateSprite(
        EditorContext& ctx,
        const std::string& textureVPath,
        const glm::vec2& position);

    static SIMPEngine::Entity CreateTilemap(EditorContext &ctx);

    static bool AssignTileset(EditorContext &ctx, SIMPEngine::Entity entity, const std::string &path);

    static SIMPEngine::Entity ImportTiledMap(EditorContext &ctx, const std::string &path);

    static SIMPEngine::Entity CreateParticleEmitter(EditorContext &ctx, const SIMPEngine::ParticleEffect &effect, const glm::vec2 &position = glm::vec2(0.0f));

    static void ApplyParticleEffect(EditorContext &ctx, SIMPEngine::Entity entity, const SIMPEngine::ParticleEffect &effect);

    static bool SaveParticleEffect(const SIMPEngine::ParticleEffect &effect, std::string &outPath);

    enum class UINodeKind
    {
        Canvas,
        Panel,
        Label,
        Image,
        Button,
        Row,
        Column
    };

    static std::string CreateShaderFile(const std::string &name);

    static SIMPEngine::Entity CreateShaderCanvas(EditorContext &ctx);

    static bool SaveAsPrefab(EditorContext &ctx, SIMPEngine::Entity entity, std::string *savedPath = nullptr);

    static bool ApplyToPrefab(EditorContext &ctx, SIMPEngine::Entity entity);

    static SIMPEngine::Entity UpdateFromPrefab(EditorContext &ctx, SIMPEngine::Entity entity);

    static SIMPEngine::Entity InstantiatePrefab(EditorContext &ctx, const std::string &path, const glm::vec2 &position);

    static SIMPEngine::Entity CreateUINode(EditorContext &ctx, UINodeKind kind);

    static SIMPEngine::Entity CreateLight(EditorContext &ctx, bool spot, const glm::vec2 &position = glm::vec2(0.0f));

    static void DeleteEntity(
        EditorContext& ctx,
        SIMPEngine::Entity entity);

    static SIMPEngine::Entity DuplicateEntity(
        EditorContext& ctx,
        SIMPEngine::Entity entity);

    static void DeleteSelection(EditorContext &ctx);

    static SIMPEngine::Entity GroupSelection(EditorContext &ctx, const std::string &name = "Group");

    static void Ungroup(EditorContext &ctx, SIMPEngine::Entity group);

    static bool IsGroup(SIMPEngine::Entity entity);

    static void DuplicateSelection(EditorContext &ctx);

    static void RenameEntity(
        EditorContext& ctx,
        SIMPEngine::Entity entity,
        const std::string& name);

    static bool ReparentEntity(
        EditorContext& ctx,
        SIMPEngine::Entity child,
        SIMPEngine::Entity parent);

    static std::shared_ptr<SIMPEngine::Texture> LoadTexture(const std::string &vpath);
};
