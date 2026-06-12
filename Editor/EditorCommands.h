#pragma once

#include "EditorContext.h"

class EditorCommands
{
public:
    static SIMPEngine::Entity CreateEntity(
        EditorContext& ctx,
        const std::string& name);

    static void DeleteEntity(
        EditorContext& ctx,
        SIMPEngine::Entity entity);

    static void RenameEntity(
        EditorContext& ctx,
        SIMPEngine::Entity entity,
        const std::string& name);
};