#include "EditorCommands.h"
#include <Engine/Scene/Component.h>

SIMPEngine::Entity EditorCommands::CreateEntity(EditorContext &ctx, const std::string &name)
{
    auto entity = ctx.Scene->CreateEntity(name);
    return entity;
}

void EditorCommands::DeleteEntity(
    EditorContext &ctx,
    SIMPEngine::Entity entity)
{
    ctx.Scene->GetRegistry().destroy(
        entity.GetHandle());

    if (ctx.SelectedEntity &&
        ctx.SelectedEntity.GetHandle() == entity.GetHandle())
    {
        ctx.SelectedEntity = {};
    }
}

void EditorCommands::RenameEntity(
    EditorContext &ctx,
    SIMPEngine::Entity entity,
    const std::string &name)
{
    entity.GetComponent<TagComponent>().Tag =
        name;
}