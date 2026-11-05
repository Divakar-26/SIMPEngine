#pragma once

#include "EditorContext.h"

#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace SIMPEngine
{
    class Texture;
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

    static void DeleteEntity(
        EditorContext& ctx,
        SIMPEngine::Entity entity);

    static SIMPEngine::Entity DuplicateEntity(
        EditorContext& ctx,
        SIMPEngine::Entity entity);

    static void DeleteSelection(EditorContext &ctx);

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
